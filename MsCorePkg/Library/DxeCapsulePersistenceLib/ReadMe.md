# Capsule Persistence Lib for PEI and DXE

This is an implementation of CapsulePersistenceLib which is used to persist capsules using the EFI partition.

It provides methods for:

- Persisting a capsule across reset
- Retrieving all currently persisted capsules and deleting them
- Retrieving a specific persisted capsule
- Deleting a specific persisted capsule

This library can be used without any sort of queue, by using a retrieve all method but, ideally, it
should be paired with `QueueLib`.
