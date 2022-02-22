# CapsuleServicePei

Produces the capsule PPI which currently only fully implements the `CheckCapsuleUpdate` functionality.

Everything else returns success or unsupported, as it is related to capsule coalescing.

This is needed as there is often infrastructure which detects FlashUpdate as a boot mode and
attempts to find this PPI to check to make sure we actually have a capsule. So, we produce a simple
PPI to satisfy that requirement.
