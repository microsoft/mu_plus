/** @file
  This library provides the _fltused symbol which is required to by MSVC and clang
  when floating point operations are used. This library should be linked in for
  any module that uses floating point operations.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// create a global to satisfy the compilers insertion of the _fltused symbol
// in response to detecting floating point type operations.
int  _fltused = 0x9875;
