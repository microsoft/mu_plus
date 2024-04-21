# Advanced Logger PRM Sample Driver

The Advanced Logger PRM sample driver demonstrates how to write a KMDF driver to utlize the Windows Platform Runtime
Mechanism (PRM) direct-call interface to talk to the [Advanced Logger PRM](../../../AdvLoggerOsConnectorPrm)
to fetch the log and write it to a file.

This driver is based on the Windows Driver Sample [PrmFuncSample driver](https://github.com/microsoft/Windows-driver-samples/tree/develop/prm).

## Building

*Note* This driver requires a Windows SDK greater than 10.0.22621 (which as of this writing is what is publicly
available) to access PRM structures that were not previously available.

Open prmsample.sln in Visual Studio 2022. Ensure you have the Windows SDK and WDK installed as well as the MSVC build
tools for your architecture. Click Build->Build Solution.

## Running

Copy the output binaries to the target system. Start the driver by opening Cmd Prompt and running:

- `sc create PrmSample binPath="{PATH_TO_BINS/prmfuncsample.sys}" type=kernel`
- `sc start PrmSample`

The driver will write the log to `C:\AdvLogger.log`. You can then use [DecodeUefiLog.py](../../DecodeUefiLog/ReadMe.md)
to decode the log.

You can unload the driver by doing:

- `sc stop PrmSample`

Further loading of the driver will fetch the current log and overwrite `C:\AdvLogger.log`.

## Consumption

This driver is only written as a sample. To access the Adv Logger PRM in your own code, copy the patterns this sample
driver does to communicate with PRM.sys. Your driver can link up to an application to be driven or be a standalone
driver.

## Related topics

[Platform Runtime Mechanism Specification](https://uefi.org/sites/default/files/resources/Platform%20Runtime%20Mechanism%20-%20with%20legal%20notice.pdf/)
