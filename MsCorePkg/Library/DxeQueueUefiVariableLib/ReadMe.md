# DXE Queue UEFI Variable Lib

This is an implementation of QueueLib backed by the UEFI variable store.

Because the queue has manipulation functions, this does not support PEI as the variable
services in PEI are usually read only.
