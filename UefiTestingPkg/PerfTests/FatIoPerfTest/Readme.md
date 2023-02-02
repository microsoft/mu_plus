# AdvLoggerPkg - PerfTest application

## About

The PerfTest application is a simple representation of the file logger in order to test the performance
cost of writing approximately 1MB of log data to the EFI System Partition.

## Configuration

The performance test will read, if it exists, a boot loader from all Simple File Systems.  In order to
test the write and read performance of the log file does require some preparation.

1. Create a PerfTest directory in the root of the ESP and any other Simple File System devices.
2. Run the FatIoPerfTest.efi one time.  This will create File2Test.txt in the PerfTest directory.
3. Copy File2Test.txt to File3Test.txt in the PerfTest directory.

After this setup, all three test should run successfully

## NOTE

In order to start with no cache in the FAT file system, the code disconnects all the controllers publishing
the SimpleFileSystem protocols, and reconnects them.  This means you cannot redirect the output to a file.

An option to cache the prints until the the end of the program and write them to a log file is planned
for the near future.
