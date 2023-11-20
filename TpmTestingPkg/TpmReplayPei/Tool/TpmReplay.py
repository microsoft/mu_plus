# @file TpmReplay.py
#
# A script to manage TPM Replay event logs.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from __future__ import annotations

from argparse import RawTextHelpFormatter
import base64
import chardet
import efi_time
import json
import jsonschema
import logging
import os
import re
import struct
import sys
import tcg_platform as tcg
import timeit
import yaml

from edk2toollib.tpm.tpm2_defs import TPM_ALG_SHA256
from tcg_platform import (
    PCR_0,
    PCR_7,
    TcgEfiSpecIdEvent,
    TcgPcrEvent,
    TcgPcrEvent2,
    TcgUefiVariableData,
    TpmlDigestValues,
    TpmtHa,
    VALUE_FROM_ALG,
    VALUE_FROM_EVENT,
    ALG_FROM_VALUE,
    EVENT_FROM_VALUE,
)

from enum import IntEnum
from pathlib import PurePath
from typing import Dict, Iterable, List, Union

PROGRAM_NAME = "TPM Replay"

# Logging constants
PERF = "[PERF]"

# Global logger instance
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)


class ExitCode(IntEnum):
    SUCCESS = 0
    GENERAL_ERROR = 100
    KBD_INTERRUPT = 101
    FILE_NOT_FOUND = 102


class QuietFilter(logging.Filter):
    """A logging filter that temporarily suppresses message output."""

    def __init__(self, quiet: bool = False):
        """Class constructor method.

        Args:
            quiet (bool, optional): Indicates if messages are currently being
            printed (False) or not (True). Defaults to False.
        """

        self._quiet = quiet

    def filter(self, record: logging.LogRecord) -> bool:
        """Quiet filter method.

        Args:
            record (logging.LogRecord): A log record object that the filter is
            applied to.

        Returns:
            bool: True if messages are being suppressed. Otherwise, False.
        """
        return not self._quiet


class CalculatedPcrState(object):
    """
    Represents the final value of a fully-extended PCR

    Contains multiple digests for algorithm agility
    """

    # struct {
    #   UINT32                  PcrIndex
    #   TPML_DIGEST_VALUES      Values
    # } CALCULATED_PCR_STATE
    _hdr_struct_format = "<I"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    def __init__(
        self, pcr_index: int, algs: Iterable = None, digests: TpmlDigestValues = None
    ):
        """Class constructor method.

        Args:
            pcr_index (int): The PCR index.
            algs (Iterable, optional): An iterable of hash algorithms. Defaults
              to None.
            digests (TpmlDigestValues,
            optional): A TpmlDigestValues object. Defaults to None.
        """
        self.pcr_index = pcr_index
        if digests is None:
            self.digest_values = tcg.TpmlDigestValues(algs)
        else:
            self.digest_values = digests

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "CALCULATED_PCR_STATE\n"
        debug_str += "\tPcrIndex    = 0x%08X\n" % self.pcr_index
        debug_str += str(self.digest_values)
        return debug_str

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        return self._hdr_struct_size + self.digest_values.get_size()

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        result = struct.pack(self._hdr_struct_format, self.pcr_index)
        result += self.digest_values.encode()
        return result

    @classmethod
    def from_binary(cls, binary_data: bytes) -> CalculatedPcrState:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            CalculatedPcrState: A CalculatedPcrState instance.
        """
        pcr_index, *_ = struct.unpack_from(
            CalculatedPcrState._hdr_struct_format, binary_data
        )
        digest_values = TpmlDigestValues.from_binary(
            binary_data[CalculatedPcrState._hdr_struct_size :]
        )
        return cls(pcr_index, digests=digest_values)


class CryptoAgileEventLog(object):
    # struct {
    #   TCG_PCClientPCREvent SpecIdEvent;               // TCG_EfiSpecIDEvent
    #   TCG_PCR_EVENT2       LogEvents[EventLogCount];
    # } CRYPTO_AGILE_EVENT_LOG
    def __init__(self, binary_data: bytes):
        """Class constructor method.

        Args:
            binary_data (bytes): Binary data containing the log.
        """
        self._event_log = []
        self.tcg_pcr_event = TcgPcrEvent.from_binary(binary_data)

        offset = self.tcg_pcr_event.get_size()
        while offset < len(binary_data):
            event = TcgPcrEvent2.from_binary(binary_data[offset:])
            offset += event.get_size()
            self._event_log.append(event)

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        spec_id_event = TcgEfiSpecIdEvent.from_binary(self.tcg_pcr_event.event)

        debug_str = "CRYPTO_AGILE_EVENT_LOG\n"
        debug_str += str(self.tcg_pcr_event)
        debug_str += str(spec_id_event)
        for event in self._event_log:
            debug_str += str(event)
        return debug_str

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        result = self.tcg_pcr_event.get_size()
        for event in self._event_log:
            result += event.get_size()
        return result

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        result = self.tcg_pcr_event.encode()
        for event in self._event_log:
            result += event.encode()
        return result


class TpmReplayEventLog(object):
    # struct {
    #   SIG64       StructureSignature
    #   UINT32      Revision
    #   EFI_TIME    Timestamp
    #   UINT32      StructureSize
    #   UINT32      FinalPcrCount
    #   UINT32      OffsetToFinalPcrs
    #   UINT32      EventLogCount
    #   UINT32      OffsetToEventLog
    #   // CALCULATED_PCR_STATE  FinalPcrs[FinalPcrCount]
    #   // TCG_PCR_EVENT2        EventLog[EventLogCount]
    # } TPM_REPLAY_EVENT_LOG
    _hdr_struct_format = "<8sI16sIIIII"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    CURRENT_REVISION = 0x00000100

    def __init__(self, binary_data: bytes = None):
        """Class constructor method.

        Args:
            binary_data (bytes, optional): A binary representation of a TPM
              Replay Event Log. Defaults to None.
        """
        if binary_data:
            (
                self.signature,
                self.revision,
                self.timestamp,
                _,
                final_pcr_count,
                offset_to_final_pcrs,
                event_log_count,
                offset_to_event_log,
            ) = struct.unpack_from(self._hdr_struct_format, binary_data)
            self.timestamp = efi_time.EfiTime.from_binary(self.timestamp)
            self._final_pcrs = []
            self._event_log = []

            offset = offset_to_final_pcrs
            for _ in range(0, final_pcr_count):
                calculated_pcr_state = CalculatedPcrState.from_binary(
                    binary_data[offset:offset_to_event_log]
                )
                offset += calculated_pcr_state.get_size()
                self._final_pcrs.append(calculated_pcr_state)

            offset = offset_to_event_log
            for _ in range(0, event_log_count):
                event = TcgPcrEvent2.from_binary(binary_data[offset:])
                offset += event.get_size()
                self._event_log.append(event)
        else:
            self.signature = b"_TPMRPL_"
            self.revision = TpmReplayEventLog.CURRENT_REVISION
            self.timestamp = efi_time.EfiTime()
            self._final_pcrs = []
            self._event_log = []

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "TPM_REPLAY_EVENT_LOG\n"
        debug_str += "\tStructureSignature  = %s\n" % self.signature
        debug_str += "\tRevision            = 0x%08X\n" % self.revision
        debug_str += "\tTimestamp           = %s\n" % self.timestamp
        debug_str += "\tFinalPcrCount       = 0x%08X\n" % len(self._final_pcrs)
        debug_str += "\tEventLogCount       = 0x%08X\n" % len(self._event_log)
        for final_pcr in self._final_pcrs:
            debug_str += str(final_pcr)
        for event in self._event_log:
            debug_str += str(event)
        return debug_str

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TpmReplayEventLog:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            TpmReplayEventLog: A TpmReplayEventLog instance.
        """
        count, *_ = struct.unpack_from(TcgPcrEvent2._hdr_struct_format, binary_data)

        digests = {}
        offset = TpmlDigestValues._hdr_struct_size
        for _ in range(0, count):
            tpmt_ha = TpmtHa.from_binary(binary_data[offset:])
            digests[tpmt_ha.hash_alg] = tpmt_ha
            offset += tpmt_ha.get_size()

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        result = self._hdr_struct_size
        for final_pcr in self._final_pcrs:
            result += final_pcr.get_size()
        for event in self._event_log:
            result += event.get_size()
        return result

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        offset_to_final_pcrs = self._hdr_struct_size

        offset_to_event_log = offset_to_final_pcrs
        for final_pcr in self._final_pcrs:
            offset_to_event_log += final_pcr.get_size()

        offset_to_signature = offset_to_event_log
        for event in self._event_log:
            offset_to_signature += event.get_size()

        result = struct.pack(
            self._hdr_struct_format,
            self.signature,
            self.revision,
            self.timestamp.encode(),
            offset_to_signature,
            len(self._final_pcrs),
            offset_to_final_pcrs,
            len(self._event_log),
            offset_to_event_log,
        )
        for final_pcr in self._final_pcrs:
            result += final_pcr.encode()
        for event in self._event_log:
            result += event.encode()
        return result

    # index = pcr index
    # data = bytes of data to hash and extend
    def extend_pcr_data(self, index: int, data: bytes):
        """Extends a PCR with data.

        Args:
            index (int): PCR index.
            data (bytes): Data.
        """
        self._final_pcrs[index].digest_values.extend(data)

    # index = pcr index
    # values = pre-hashed digest values to extend
    def extend_pcr_digest(self, index: int, values: TpmlDigestValues):
        """Extends a PCR with a given set of digests.

        Args:
            index (int): PCR index
            values (TpmlDigestValues): The digest values.
        """
        for alg, digest in values.items():
            self._final_pcrs[index].digest_values.digests[alg].extend_digest(
                digest.HashDigest
            )


def _get_event(
    pcr: int, event: str, data: bytes, hash_algs: tuple = (TPM_ALG_SHA256,)
) -> TcgPcrEvent2:
    """Gets a event log event.

    Args:
        pcr (int): PCR index.
        event (str): Event name.
        data (bytes): Event data.
        hash_algs (tuple, optional): Hash algorithms supported. Defaults to
          (TPM_ALG_SHA256,).

    Returns:
        TcgPcrEvent2: A TCG PCR Event 2 event.
    """
    logger.debug(f" PCR[{pcr}]: Adding event {event}.")

    new_event = tcg.TcgPcrEvent2(hash_algs)
    new_event.pcr_index = pcr
    new_event.event_type = VALUE_FROM_EVENT[event]
    new_event.digest_values.set_hash_data(data)
    new_event.event = data
    return new_event


# dictionary key = hash algorithm
# dictionary value = corresponding hash for the the given algorithm
def _get_event_pre_hashed_data(
    pcr: int, event: str, data: bytes, algs_and_hash: Dict[int, str]
):
    """Gets an event that contains pre-hashed data.

    Args:
        pcr (int): PCR index.
        event (str): Event name.
        data (bytes): Event data.
        algs_and_hash (Dict[int, str]): A dictionary of hash algorithms
          associated with a hexadecimal string digest.

    Returns:
        _type_: _description_
    """
    logger.debug(f"  PCR[{pcr}]: Adding event {event}.")

    new_event = tcg.TcgPcrEvent2([*algs_and_hash])
    new_event.pcr_index = pcr
    new_event.event_type = VALUE_FROM_EVENT[event]
    new_event.event = data

    for alg, hash in algs_and_hash.items():
        new_event.digest_values.digests[alg] = TpmtHa(hex(alg), bytes.fromhex(hash))

    return new_event


def _get_digest_dict(string_dict: Dict[str, str]) -> Dict[int, str]:
    """Returns a dictionary of algorithm IDs associated with hexadecimal
    strings.

    Args:
        string_dict (Dict[str,str]): A dictionary of hash algorithm string
        and hexadecimal string pairs.

    Returns:
        Dict[int,str]: A dictionary of hash algorithm ID and hexadecimal string
        pairs where the 0x prefix is stripped from the hexadecimal string.
    """
    return {VALUE_FROM_ALG[k]: v[2:] for k, v in string_dict.items()}


def _process_event_data(event_data: Dict[str, str]) -> bytes:
    """Returns a bytes object for a given dictionary describing event data.

    Args:
        event_data (Dict[str, str]): A dictionary describing event data.

    Returns:
        bytes: A byte object representation of the event data.
    """
    if event_data["type"] == "string":
        data = event_data["value"].encode("utf-8")
        if "include_null_char" in event_data and event_data["include_null_char"]:
            data += b"\0"
        if "encoding" in event_data and event_data["encoding"] != "utf-8":
            data = data.decode("utf-8").encode(
                event_data["encoding"].replace("utf-16", "utf-16le")
            )
    elif event_data["type"] == "base64":
        data = base64.b64decode(event_data["value"])
    elif event_data["type"] == "variable":
        var_name = tuple(
            int(x, 0)
            for x in re.findall(r"0x[0-9A-Fa-f]+", event_data["variable_name"])
        )
        tcg_var = TcgUefiVariableData(
            var_name,
            event_data["variable_unicode_name_length"],
            event_data["variable_data_length"],
            event_data["variable_unicode_name"],
            base64.b64decode(event_data["value"]),
        )
        data = tcg_var.encode()

    return data


def _build_tpm_replay_event_log_from_yaml(
    yaml_data: Dict[str, List[Union[str, Dict[str, "Dict"]]]]
) -> TpmReplayEventLog:
    """Builds a TPM Replay Event Log from YAML.

    Args:
        yaml_data (Dict[str, List[Union[str, Dict[str, 'Dict']]]]): A dictionary
        where keys are string and values are either strings or additional
        nested dictionaries.

    Returns:
        TpmReplayEventLog: A TpmReplayEventLog instance built from the given
        dictionary.
    """
    pcr_state = dict.fromkeys(range(PCR_0, PCR_7 + 1))
    replay_event_log = TpmReplayEventLog()

    def _add_digest(pcr: int, alg: int, digest: bytes, prehashed: bool = False) -> None:
        """Adds a digest to the given PCR.

        Args:
            pcr (int): PCR index.
            alg (int): Algorithm ID.
            digest (bytes): Bytes object of the digest value.
            prehashed (bool, optional): Whether the digest is prehashed.
              Defaults to False.
        """
        if pcr > 7:
            logger.debug(
                "Skipping calculation of a PCR state greater than 7 " f"({pcr})."
            )
            return

        logger.debug(
            f"    PCR[{pcr}]: Adding {ALG_FROM_VALUE[alg]} digest " f"{str(digest)}."
        )

        if pcr_state[pcr] is None:
            pcr_state[pcr] = {}

        if alg not in pcr_state[pcr]:
            pcr_state[pcr][alg] = TpmtHa(alg)

        if prehashed:
            pcr_state[pcr][alg].extend_digest(digest)
        else:
            pcr_state[pcr][alg].extend_data(digest)

    logger.debug("Processing events...")
    for event in yaml_data["events"]:
        data = _process_event_data(event["data"])

        if "prehash" in event:
            new_event = _get_event_pre_hashed_data(
                event["pcr"], event["type"], data, _get_digest_dict(event["prehash"])
            )
            for alg, digest in new_event.digest_values.digests.items():
                _add_digest(event["pcr"], alg, digest.hash_digest, True)
        else:
            algs = tuple(VALUE_FROM_ALG[h] for h in event["hash"])
            new_event = _get_event(event["pcr"], event["type"], data, algs)
            for alg in algs:
                _add_digest(event["pcr"], alg, data, False)

        replay_event_log._event_log.append(new_event)

    logger.debug("Assembling final PCR states...")
    for pcr_index, pcr_digests in pcr_state.items():
        tpml_digest_values = TpmlDigestValues()
        if pcr_digests is not None:
            logger.debug(f"  PCR[{pcr_index}]:.")
            for alg, digest in pcr_digests.items():
                logger.debug(f"    {ALG_FROM_VALUE[alg]} present.")
                tpml_digest_values.digests[alg] = digest
            calc_pcr_state = CalculatedPcrState(pcr_index)
            calc_pcr_state.digest_values = tpml_digest_values
            replay_event_log._final_pcrs.append(calc_pcr_state)

    return replay_event_log


def _build_yaml_from_event_log(
    event_log: TpmReplayEventLog,
) -> Dict[str, List[Union[str, "Dict"]]]:
    """Builds the YAML represenation of a binary event log.

    Args:
        event_log (EventLog): An Event Log object.

    Returns:
        Dict[str, List[Union[str, 'Dict']]]: A dictionary that contains either
        a string or additional nested dictionaries.

    """
    import tcg_platform

    yaml_data = {"events": []}

    logger.debug("Processing events...")
    for event in event_log._event_log:
        event_data = {}
        event_data["type"] = EVENT_FROM_VALUE[event.event_type]
        event_data["pcr"] = event.pcr_index
        event_data["prehash"] = {}

        logger.debug(f"  PCR[{event_data['pcr']}]: {event_data['type']}:")

        for alg, digest in event.digest_values.digests.items():
            logger.debug(f"    {ALG_FROM_VALUE[alg]} present.")
            event_data["prehash"][ALG_FROM_VALUE[alg]] = "0x" + "".join(
                [f"{b:02x}" for b in digest.hash_digest]
            )

        event_data["data"] = {}

        if event.event_type in (
            tcg_platform.EV_EFI_VARIABLE_DRIVER_CONFIG,
            tcg_platform.EV_EFI_VARIABLE_BOOT,
            tcg_platform.EV_EFI_VARIABLE_BOOT2,
            tcg_platform.EV_EFI_VARIABLE_AUTHORITY,
        ):
            event_data["data"]["type"] = "variable"
            tcg_var = TcgUefiVariableData.from_binary(event.event)
            event_data["data"]["variable_name"] = tcg_var.guid
            event_data["data"][
                "variable_unicode_name_length"
            ] = tcg_var.unicode_name_length
            event_data["data"]["variable_data_length"] = tcg_var.variable_data_length
            event_data["data"]["variable_unicode_name"] = tcg_var.unicode_name
            event_data["data"]["value"] = base64.b64encode(
                tcg_var.variable_data
            ).decode("utf-8")
        else:
            char_result = chardet.detect(event.event)

            if char_result["encoding"] == "ascii" and all(
                event.event[i] == 0 for i in range(1, len(event.event), 2)
            ):
                char_result["encoding"] = "utf-16le"

            event_value = event.event
            if char_result["encoding"] == "ascii" or char_result["encoding"] == "utf-8":
                event_data["data"]["type"] = "string"
                event_data["data"]["encoding"] = "utf-8"
                if event_value[-1:] == b"\0":
                    event_value = event_value[:-1]
                    event_data["data"]["include_null_char"] = True
                event_data["data"]["value"] = event_value.decode("utf-8")
            elif char_result["encoding"] == "utf-16le":
                event_data["data"]["type"] = "string"
                event_data["data"]["encoding"] = "utf-16"
                if event_value[-2:] == b"\x00\x00":
                    event_value = event_value[:-2]
                    event_data["data"]["include_null_char"] = True
                event_data["data"]["value"] = event_value.decode("utf-16")
            else:
                event_data["data"]["type"] = "base64"
                event_data["data"]["value"] = base64.b64encode(event_value).decode(
                    "utf-8"
                )

        logger.debug(f"    {event_data['data']['type']} event detected.")

        yaml_data["events"].append(event_data)

    return yaml_data


def _begin() -> int:
    """Provides a command-line argument wrapper.

    Returns:
        int: The system exit code value.
    """
    import argparse
    import builtins

    def _check_file_path(file_path: str) -> bool:
        """Returns the absolute path if the path is a file."

        Args:
            file_path (str): A file path.

        Raises:
            FileExistsError: The path is not a valid file.

        Returns:
            bool: True if the path is a valid file else False.
        """
        abs_file_path = os.path.abspath(file_path)
        if os.path.isfile(file_path):
            return abs_file_path
        else:
            raise FileExistsError(file_path)

    def _quiet_print(*args, **kwargs):
        """Replaces print when quiet is requested to prevent printing messages."""
        pass

    stdout_logger_handler = logging.StreamHandler(sys.stdout)
    stdout_logger_handler.set_name("stdout_logger_handler")
    stdout_logger_handler.setLevel(logging.INFO)
    stdout_logger_handler.setFormatter(logging.Formatter("%(message)s"))
    logger.addHandler(stdout_logger_handler)

    parser = argparse.ArgumentParser(
        prog=PROGRAM_NAME,
        description=(
            "Performs operations related to managing TPM "
            "event logs.\n\nPrimarily intended to support the "
            "TPM Replay firmware feature."
        ),
        formatter_class=RawTextHelpFormatter,
    )

    input_file_group = parser.add_mutually_exclusive_group(required=True)
    output_file_group = parser.add_argument_group("Output options")
    logging_group = parser.add_argument_group("Logging options")

    input_file_group.add_argument(
        "-i",
        "--input-desc-file",
        type=_check_file_path,
        help="An input description (JSON or YAML) file that\ncontains the TCG "
        "event log information to process.\n\n",
    )

    input_file_group.add_argument(
        "-e",
        "--input-event-log-file",
        type=_check_file_path,
        help="An input TPM replay event log file that "
        "was\npreviously created by this tool to "
        "process.\n\n",
    )

    output_file_group.add_argument(
        "-o",
        "--output-file",
        required=True,
        help="The output file path. If a YAML file "
        "was given\nas input, a binary TPM replay "
        "will be output.\nIf a binary TPM event log "
        "was given as input,\na YAML file will be "
        "output.\n\n",
    )

    output_file_group.add_argument(
        "-r",
        "--output-report",
        action="store_true",
        help="If present, a report of the TCG "
        "event log\ninformation will be output "
        "alongside the output\nfile. The file will "
        "have the same name as the\noutput file "
        "with a -report.txt suffix.\n\n",
    )

    logging_group.add_argument(
        "-l",
        "--log-file",
        nargs="?",
        default=None,
        const="tpm_replay.log",
        help="File path for log output.\n"
        "(default: if the flag is given with no "
        "file path\nthen a file called "
        "tpm_replay.log is created and\nused "
        "in the current directory)\n\n",
    )

    logging_group.add_argument(
        "-v",
        "--verbose-log-file",
        action="count",
        default=0,
        help="Set file logging verbosity level.\n"
        " - None:    Info & > level messages\n"
        " - '-v':    + Debug level messages\n"
        " - '-vv':   + File name and function\n"
        " - '-vvv':  + Line number\n"
        " - '-vvvv': + Timestamp\n"
        "(default: verbose logging is not enabled)"
        "\n\n",
    )

    logging_group.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Disables console output.\n" "(default: console output is enabled)\n\n",
    )

    args = parser.parse_args()

    if args.quiet:
        # Don't print anywhere the may directly print
        builtins.print = _quiet_print
    stdout_logger_handler.addFilter(QuietFilter(args.quiet))

    if args.log_file:
        file_logger_handler = logging.FileHandler(
            filename=args.log_file, mode="w", encoding="utf-8"
        )

        if args.verbose_log_file == 0:
            file_logger_handler.setLevel(logging.INFO)
            file_logger_formatter = logging.Formatter("%(levelname)-8s %(message)s")
        elif args.verbose_log_file == 1:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter("%(levelname)-8s %(message)s")
        elif args.verbose_log_file == 2:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                "[%(filename)s - %(funcName)20s() ] %(levelname)-8s " "%(message)s"
            )
        elif args.verbose_log_file == 3:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                "[%(filename)s:%(lineno)s - %(funcName)20s() ] "
                "%(levelname)-8s %(message)s"
            )
        elif args.verbose_log_file == 4:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                "%(asctime)s [%(filename)s:%(lineno)s - %(funcName)20s() ]"
                " %(levelname)-8s %(message)s"
            )
        else:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                "%(asctime)s [%(filename)s:%(lineno)s - %(funcName)20s() ]"
                " %(levelname)-8s %(message)s"
            )

        file_logger_handler.setFormatter(file_logger_formatter)
        logger.addHandler(file_logger_handler)

    logger.info(PROGRAM_NAME + "\n")

    with open("TpmReplaySchema.json", "r") as s:
        json_schema = json.load(s)

    if args.input_desc_file:
        logger.info(f"Reading input description file {args.input_desc_file}")

        desc_path = PurePath(args.input_desc_file)
        with open(desc_path, "r") as idf:
            if desc_path.suffix.lower() in [".yaml", ".yml"]:
                file_data = yaml.safe_load(idf)
            else:
                file_data = json.load(idf)

        start_time = timeit.default_timer()
        try:
            jsonschema.validate(instance=file_data, schema=json_schema)
        except jsonschema.ValidationError as e:
            # Allow PCRs greater than 7 on input.
            if (
                "pcr" in e.schema_path
                and e.validator == "maximum"
                and e.validator_value == 7
            ):
                pass
            else:
                raise e
        end_time = timeit.default_timer() - start_time
        logger.debug(f"{PERF} JSON schema validation took {end_time:.2f} seconds.")

        start_time = timeit.default_timer()
        log = _build_tpm_replay_event_log_from_yaml(file_data)
        end_time = timeit.default_timer() - start_time
        logger.debug(
            f"{PERF} Total event log creation time: " f"{end_time:.2f} seconds."
        )

        with open(args.output_file, "wb") as output_replay_event_log:
            output_replay_event_log.write(log.encode())

        logger.info(f"Output: Binary file {args.output_file}")

    elif args.input_event_log_file:
        logger.info(f"Reading input binary file {args.input_event_log_file}")

        with open(args.input_event_log_file, "rb") as log_file:
            binary_data = log_file.read()

        start_time = timeit.default_timer()
        if binary_data[:8] == b"_TPMRPL_":
            logger.debug("Input: Binary log file recognized as a TPM Replay log.")
            log = TpmReplayEventLog(binary_data)
        else:
            logger.debug("Input: Binary log file recognized as a Crypto Agile log.")
            log = CryptoAgileEventLog(binary_data)

        data = _build_yaml_from_event_log(log)
        json_obj = json.loads(json.dumps(data))
        try:
            jsonschema.validate(instance=json_obj, schema=json_schema)
        except jsonschema.ValidationError as e:
            # Allow PCRs greater than 0 if converting a log other than the TPM
            # Replay event log since it will contain all PCR values not only
            # UEFI firmware produced values.
            if (
                "pcr" in e.schema_path
                and e.validator == "maximum"
                and e.validator_value == 7
                and isinstance(log, CryptoAgileEventLog)
            ):
                pass
            else:
                raise e
        yaml_data = yaml.dump(
            json_obj, default_flow_style=False, indent=2, sort_keys=False
        )
        end_time = timeit.default_timer() - start_time
        logger.debug(
            f"{PERF} Total time spent converting the binary to YAML: "
            f"{end_time:.2f} seconds."
        )

        with open(args.output_file, "w") as output_yaml_file:
            output_yaml_file.write(yaml_data)

        logger.info(f"Output: YAML file {args.output_file}")

    if args.output_report:
        report_path = PurePath(args.output_file)
        report_path = report_path.parent / (report_path.name + "-report.txt")
        with open(report_path, "w") as report_file:
            report_file.write(str(log))
        logger.info(f"Output: Report file {report_path}")

    return ExitCode.SUCCESS


if __name__ == "__main__":
    # Some systems require the return value to be in the range 0-127, so
    # a lower maximum of 100 is enforced to allow a wide range of potential
    # values with a reasonably large maximum.
    try:
        sys.exit(min(_begin(), ExitCode.GENERAL_ERROR))
    except KeyboardInterrupt:
        logger.warning("Exiting due to keyboard interrupt.")
        sys.exit(ExitCode.KBD_INTERRUPT)
    except FileExistsError as e:
        logger.critical(f"Input file {e.args[0]} does not exist.")
        sys.exit(ExitCode.FILE_NOT_FOUND)
