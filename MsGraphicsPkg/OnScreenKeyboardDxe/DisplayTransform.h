/** @file
  Implements structures and constants used by display transformation routines for a simple on-screen virtual keyboard.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
 
#ifndef _DISPLAY_TRANSFORM_H_
#define _DISPLAY_TRANSFORM_H_

// Function prototypes
//
VOID InitializeXformWithParams (float ScaleFactor,
                                float Xang,
                                float Yang,
                                float Zang
                               );

VOID TransformPointSet (POINT3D *inset,
                        POINT3D *outset,
                        UINTN count);

POINT3D TransformPoint (POINT3D inpt);

VOID MatrixMult (float destination[4][4],
                 float source[4][4]);

VOID Translate  (float dx,
                 float dy,
                 float dz);

VOID Scale      (float factor);
VOID RotateX    (float ang);  
VOID RotateY    (float ang);  
VOID RotateZ    (float ang);  

#endif  // _DISPLAY_TRANSFORM_H_
