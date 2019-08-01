# DMAR Table Audit

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

## About

**DMARTableAudit.py**


***Unit test that checks:***
1.  DMA remapping bit is enabled
2.  No ANDD structures are included in DMAR table
3.  RMRRs are limited to only the RMRRs specified in provided XML file (if no XML provided then verify no RMRRs exist)



***Software Requirements***
1. Python3
2. Pywin32
    
    ```pip install Pywin32```
3. Project Mu python library

    ```pip install mu-python-library```

