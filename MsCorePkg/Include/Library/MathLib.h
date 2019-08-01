/** @file
This library supports basic math operations such as SQRT, sin, cos


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MATH_LIB_H__
#define __MATH_LIB_H__

#define MU_PI 3.1415926535897932384626433832

/**
Find sine of a provided double in radians

@param[in] input angle to calculate in radians

@retval the result (double)
**/
double
EFIAPI
sin_d(
  IN CONST double angleInRadians  
);


/**
Find cosine of a provided double in radians

@param[in]  angle to calculate in radians

@retval the result (double)
**/
double
EFIAPI
cos_d(
  IN CONST double angleInRadians
);

/**
Find square root of the provided double

@param[in] input the number to square root

@retval result when input >0 otherwise returns input
**/
double
EFIAPI
sqrt_d(
  IN CONST double input
);


/**
Find square root of the provided unsigned integer

@param[in] input the number to square root

@retval result when input >0 otherwise returns input
**/
UINT32
EFIAPI
sqrt32(
  IN CONST UINT32 input
);

/**
Find square root of the provided unsigned 64bit integer

@param[in] input the number to square root

@retval result when input >0 otherwise returns input
**/
UINT64
EFIAPI
sqrt64(
  IN CONST UINT64 input
);

#endif
