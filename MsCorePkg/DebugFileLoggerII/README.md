# Debug File Logger II

This is Debug File Logger II.

Debug File Logger was tried a few times with varying degrees of success.
It was thought that the not quite right USB stack was the issue with the Debug File Logger.
When the Debug File Logger was enabled to write logs in the ESP (EFI System Partition) on the NVM/E drive, there would be systems that would no longer boot.

Over time, we have found that there were issues in the Windows FAT file system driver across hibernate where the OS assumed no change to the file system metadata (The FAT and Directory Entries) causing irreparable damage to the ESP.

## Top design points.

1. The debug log files are pre-allocated the first time they are needed on a particular device.

2. Every SimpleFileSystem device that is connected during POST is eligible to be a log device.

3. A USB device must have a directory in the root named 'Logs' to be considered a log device.

4. The NVM/E device(s) will get a hidden directory named 'Logs' the first time is is mounted.  The time to create the initial logs is about 2 seconds.  The time to register the logs during subsequent time through POST is about 2ms.

5. The logs can be obtained in the OS or by booting to the Shell.  In the OS, mount the ESP using:

    mountvol P: /S

    The drive letter can be any available drive letter.

6. Logs will be recorded at:
    * When a registered device is connected
    * When just prior to ExitBootServices to any previously registered devices
    * When the system is reset (from TPL <= TPL_CALLBACK) to any previously registered devices

If you want to collect logs on a USB device, you can insert the non-bootable USB drive with the Logs directory installed, then power on the system and hold VOL/- to attempt booting from USB.  This will mount the USB filesystem, the Logs directory will be acknowledged, and the log written.  The system will continue to boot and should append the rest of the log at ExitBootServices.

# Future items

Let me know what future items are necessary.  Some under considerations:

1. Refactor DebugLib again to insure logs are collected during the whole boot.

2. Add a new feature to DebugLib to capture the full UEFI log, but only send selected entries to the serial port (to handle devices with a slow serial port).

3. Always connect the NVM/E drive at console connect.  For 99.9% of the cases, the system is going to boot the NVM/E drive, so connecting it early would not significantly change the boot time, but would allow logs to be collected in more cases.

4. When the log cannot be written (too high of TPL for the FileSystem to work), have a method to store the last 100 lines or so of the log somewhere (system dependent) that can be collected on the next boot.

5. Timestamp the head of the log to assist in sorting logs

# Installation Instructions

Replace the current Pei/DebugFileLoggerPei.inf and /Dxe/DebugFileLogger.inf with the new versions.  Delete the DebugFileLoggerLib as it is no longer necessary.

1. Remove these lines of the old Debug File Logger:

  DebugFileLoggerLib|XxxxxPkg/Library/DebugFileLoggerLib/DebugFileLoggerLib.inf
  XxxxxPkg/DebugFileLogger/Pei/DebugFileLoggerPei.inf {
  <LibraryClasses>
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }
  XxxxxPkg/DebugFileLogger/Dxe/DebugFileLogger.inf

2. Replace the old Debug file logger files with these new Debug File Logger II entries:

  MsCorePkg/DebugFileLoggerII/Pei/DebugFileLoggerPei.inf {
  <LibraryClasses>
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }

  MsCorePkg/DebugFileLoggerII/Dxe/DebugFileLogger.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }

3. There is no check for Manufacturing mode.  The idea is for the File Logger to be robust enough to be on in all builds.