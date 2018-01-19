/**

Copyright (c) 2016, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#ifndef VARIABLE_SUPPORT_H_
#define VARIABLE_SUPPORT_H_

#define DEFAULT_VERSION 0x1
#define DEFAULT_LOWESTSUPPORTEDVERSION 0x0
#define DEFAULT_LASTATTEMPT 0x0

#define VARNAME_VERSION L"FmpVersion"
#define VARNAME_LSV L"FmpLsv"

#define VARNAME_LASTATTEMPTSTATUS L"LastAttemptStatus"
#define VARNAME_LASTATTEMPTVERSION L"LastAttemptVersion"


/*
Function used to Get the FMP version from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetVersionFromVariable();

/*
Function used to Get the FMP Lowest supported version from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetLowestSupportedVersionFromVariable();

/*
Function used to Get the FMP capsule Last Attempt Status from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetLastAttemptStatusFromVariable();

/*
Function used to Get the FMP capsule Last Attempt Version from a UEFI variable.
This will return a default value if variable doesn't exist.
*/
UINT32
GetLastAttemptVersionFromVariable();


/*
Function used to Set the FMP version to a UEFI variable.

*/
VOID
SetVersionInVariable(UINT32 v);

/*
Function used to Set the FMP lowest supported version to a UEFI variable.

*/
VOID
SetLowestSupportedVersionInVariable(UINT32 v);

/*
Function used to Set the FMP Capsule Last Attempt Status to a UEFI variable.

*/
VOID
SetLastAttemptStatusInVariable(UINT32 s);


/*
Function used to Set the FMP Capsule Last Attempt Version to a UEFI variable.

*/
VOID
SetLastAttemptVersionInVariable(UINT32 v);

/*
Function locks the variables so they can't be tampered with
*/
VOID
LockAllVars();


#endif
