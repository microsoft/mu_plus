##
# Helper module to serialize, deserialize, and interact with an EFI_TIME
# structure.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from __future__ import annotations

import struct
from datetime import datetime


class EfiTime(object):
    # typedef struct {
    # UINT16 Year;          // 1900 – 9999
    # UINT8  Month;         // 1 – 12
    # UINT8  Day;           // 1 – 31
    # UINT8  Hour;          // 0 – 23
    # UINT8  Minute;        // 0 – 59
    # UINT8  Second;        // 0 – 59
    # UINT8  Pad1;
    # UINT32 Nanosecond;    // 0 – 999,999,999
    # INT16  TimeZone;      // -1440 to 1440 or 2047
    # UINT8  Daylight;
    # UINT8  Pad2;
    # } EFI_TIME;
    _struct_string = "<HBBBBBBLhBB"
    _struct_size = struct.calcsize(_struct_string)

    EFI_TIME_ADJUST_DAYLIGHT = 0x01
    EFI_TIME_IN_DAYLIGHT = 0x02

    EFI_UNSPECIFIED_TIMEZONE = 0x07FF

    year = 1900
    month = 1
    day = 1
    hour = 0
    minute = 0
    second = 0
    nanosecond = 0
    timezone = EFI_UNSPECIFIED_TIMEZONE
    daylight = EFI_TIME_ADJUST_DAYLIGHT

    def __init__(self, starting_time: datetime = None):
        """Class constructor method.

        Args:
            starting_time (datetime, optional): A datetime object. Defaults to None.
        """
        if starting_time is None:
            self.set_datetime(datetime.now())
        else:
            self.set_datetime(starting_time)

    def __str__(self):
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        return str(self.get_datetime())

    def __repr__(self) -> str:
        """Returns a string representation of this object.

        Returns:
            str: The string representation of this object.
        """
        return "EfiTime %s" % self

    def __setattr__(self, name: str, value: str) -> None:
        """Sets the given attribute to the given value.

        Args:
            name (str): Attribute name.
            value (str): Attribute value.

        Raises:
            AttributeError: The attribute is invalid.
            ValueError: The value is not valid for the given attribute.
        """
        valid_attr = (
            "year",
            "month",
            "day",
            "hour",
            "minute",
            "second",
            "nanosecond",
            "timezone",
            "daylight",
        )
        if name not in valid_attr:
            raise AttributeError(f"'{name}' is not a valid attribute!")

        int_value = int(value)
        valid = True
        if name == "year":
            if int_value < 1900 or 9999 < int_value:
                valid = False
        elif name == "month":
            if int_value < 1 or 12 < int_value:
                valid = False
        elif name == "day":
            if int_value < 1 or 31 < int_value:
                valid = False
        elif name == "hour":
            if int_value < 0 or 23 < int_value:
                valid = False
        elif name == "minute":
            if int_value < 0 or 59 < int_value:
                valid = False
        elif name == "second":
            if int_value < 0 or 59 < int_value:
                valid = False
        elif name == "nanosecond":
            if int_value < 0 or 999_999_999 < int_value:
                valid = False
        elif name == "timezone":
            if (
                int_value < -1440 or 1440 < int_value
            ) and int_value != EfiTime.EFI_UNSPECIFIED_TIMEZONE:
                valid = False
        elif name == "daylight":
            # daylight is a bitmask.
            if (
                int_value < 0
                or (EfiTime.EFI_TIME_ADJUST_DAYLIGHT | EfiTime.EFI_TIME_IN_DAYLIGHT)
                < int_value
            ):
                valid = False

        if not valid:
            raise ValueError(f"'{value}' is an invalid value for '{name}'!")

        super.__setattr__(self, name, int_value)

    def decode(self, buffer: bytes) -> bytes:
        """Decodes the given buffer.

        Args:
            buffer (bytes): The buffer.

        Returns:
            bytes: The decoded buffer.
        """
        _buffer = buffer[: self._struct_size]

        (
            self.year,
            self.month,
            self.day,
            self.hour,
            self.minute,
            self.second,
            _,
            self.nanosecond,
            self.timezone,
            self.daylight,
            _,
        ) = struct.unpack(self._struct_string, _buffer)

        return buffer[self._struct_size :]

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        return struct.pack(
            self._struct_string,
            self.year,
            self.month,
            self.day,
            self.hour,
            self.minute,
            self.second,
            0,
            self.nanosecond,
            self.timezone,
            self.daylight,
            0,
        )

    @classmethod
    def from_binary(cls, binary_data: bytes) -> EfiTime:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            EfiTime: A EfiTime instance.
        """
        timestamp = struct.unpack(EfiTime._struct_string, binary_data)
        return cls(datetime(*(timestamp[:6] + tuple([timestamp[7] // 1000]))))

    def set_datetime(self, value: datetime) -> EfiTime:
        """Sets the instance to the given datetime.

        Args:
            value (datetime): A datetime object.

        Returns:
            EfiTime: The EfiTime instance after assignment.
        """
        self.year = value.year
        self.month = value.month
        self.day = value.day
        self.hour = value.hour
        self.minute = value.minute
        self.second = value.second
        self.nanosecond = value.microsecond * 1000
        return self

    def get_datetime(self) -> datetime:
        """Returns the datetime for the current instance.

        Returns:
            datetime: The datetime instance.
        """
        us = self.nanosecond // 1000
        value = datetime(
            self.year, self.month, self.day, self.hour, self.minute, self.second, us
        )
        return value
