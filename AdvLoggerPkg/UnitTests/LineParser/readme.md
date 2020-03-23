# Verify Line Parser function for development

This test is to model the Line Parser in the AdvancedFileLogger.

The internal message log is in units of DEBUG(()).  While most messages are a complete
and end with a '\n', there are some debug messages that are built with multiple DEBUG(())
operations.

The line parser builds a debug line by copying one or more DEBUG(()) segments into a line,
and prepends the time stamp.


## About

These tests verify that the LineParser is functional.

## LineParserTestApp



## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
