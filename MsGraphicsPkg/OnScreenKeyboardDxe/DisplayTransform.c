
/** @file
  Implements display transformation routines for a simple on-screen virtual keyboard.

  Copyright (c) 2015 - 2018, Microsoft Corporation.
  
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

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MathLib.h>
#include "DisplayTypes.h"
#include "DisplayTransform.h"

float CompositeMatrix[4][4];

/**
    Initialize display transform matrix with pre-selected scaling and X/Y/Z angles.

    @param[in]  ScaleFactor  Scale multiplication factor (0 -> 1).
    @param[in]  Xang         X-axis angle in radians.
    @param[in]  Yang         Y-axis angle in radians.
    @param[in]  Zang         Z-axis angle in radians.

    @retval     None.

**/
VOID
InitializeXformWithParams (IN float ScaleFactor,
                           IN float Xang,
                           IN float Yang,
                           IN float Zang
                          )
{
	
    // Initialize composite xform matrix
    //
    SetMem(CompositeMatrix, sizeof(float)*16, 0);
    CompositeMatrix[0][0]=CompositeMatrix[1][1]=CompositeMatrix[2][2]=CompositeMatrix[3][3]=1.0;
	
    // Set default scale, rotation, translation, and perspective
    //
    Scale(ScaleFactor);
    RotateX(Xang);
    RotateY(Yang);
    RotateZ(Zang);
    Translate(0.0, 0.0, 0.0);
	
    return;
}


/**
    Transforms the specified 3D point based on the current transform matrix.

    @param[in]  InPoint     3D point to be transformed.

    @retval     Transformed 3D point.

**/
POINT3D
TransformPoint (IN POINT3D InPoint)
{
    int j, k;
    POINT3D pt = {{0.0, 0.0, 0.0, 0.0}};

    // Apply scale, rotation, and translation transform (multiplication)
    //
    for (j=0; j<4; j++)
    {
        for (k=0; k<4; k++)
        {
            pt.mtx[j]+=InPoint.mtx[k]*CompositeMatrix[j][k];
        }
    }

    return pt;
}


/**
    Transforms the specified 3D pointset based on the current transform matrix.

    @param[in]  InPointSet      3D pointset to be transformed.
    @param[out] OutPointSet     Transformed 3D pointset.
    @param[in]  PointCount      Number of points in the pointset.

    @retval     None.

**/
VOID
TransformPointSet (IN  POINT3D *InPointSet,
                   OUT POINT3D *OutPointSet,
                   IN  UINTN   PointCount)
{
    UINTN i;
	
    if (NULL == InPointSet || NULL == OutPointSet)
    {
        return;
    }
	
    // Clean out the transformed pointset array
    //
    SetMem(OutPointSet, PointCount * sizeof(POINT3D), 0);
	
    // Apply composite transformation to points and put results in transpoints array
    //
    for (i=0; i<PointCount; i++)
    {
        OutPointSet[i] = TransformPoint (InPointSet[i]);
    }
}


/**
    Multiplies two matrices and puts the result in the destination matrix.

    @param[in/out]  Destination     On input this contains one of the matrices to be multiplied and on output it holds the result. 
    @param[in]      Source          One of the matrices to be multiplied.

    @retval     None.

**/
VOID 
MatrixMult (IN OUT float Destination[4][4],
            IN     float Source[4][4])
{
    UINTN i, j, k;
    float temp[4][4];
	
    // Initialize temp matrix
    //
    SetMem(temp, sizeof(float)*16, 0);

    // Multiply destination and source matrices and store the results in the temp matrix
    //
    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            for (k=0; k<4; k++)
                temp[j][i]+=Source[k][i]*Destination[j][k];
        }
    }
	
    // Replace the contents of the destination matrix with the temp matrix multiplication values
    //
    CopyMem(Destination, temp, sizeof(float)*16);
}


/**
    Applies a 3D translation to the current transform matrix.

    @param[in]      dx      X-axis offset to be applied.
    @param[in]      dy      Y-axis offset to be applied.
    @param[in]      dz      Z-axis offset to be applied.

    @retval     None.
**/
VOID
Translate (IN float dx,
           IN float dy,
           IN float dz)
{	
    CompositeMatrix[0][3]+=dx;
    CompositeMatrix[1][3]+=dy;
    CompositeMatrix[2][3]+=dz;
    CompositeMatrix[3][3]=1.0;
}


/**
    Applies a 3D scaling factor to the current transform matrix.

    @param[in]      ScaleFactor     Scale factor to be applied.

    @retval     None.
**/
VOID
Scale (IN float ScaleFactor)
{	
    float scale[4][4];

    SetMem(scale, sizeof(float)*16, 0);
	
    // Fill in matrix with scaling factor
    //
    scale[0][0]=scale[1][1]=scale[2][2]=ScaleFactor;
    scale[3][3]=1.0;
	
    // Update the xform matrix
    //
    MatrixMult (CompositeMatrix, scale);
}


/**
    Applies a 3D scaling rotation about the X-axis to the current transform matrix.

    @param[in]  Angle       Incremental rotation angle about the X-axis in radians.

    @retval     None.
**/
VOID
RotateX (IN float Angle)
{	
    float rotex[4][4];

    SetMem(rotex, sizeof(float)*16, 0);
	
    // Fill in rotation matrix with x-axis rotation values
    //
    rotex[0][0]=1.0;
    rotex[1][1]=(float)cos_d(Angle);
    rotex[2][1]=(float)sin_d(Angle);
    rotex[1][2]=(float)-sin_d(Angle);
    rotex[2][2]=(float)cos_d(Angle);
    rotex[3][3]=1.0;
	
    // Update the xform matrix
    //
    MatrixMult (CompositeMatrix, rotex);
}


/**
    Applies a 3D scaling rotation about the Y-axis to the current transform matrix.

    @param[in]  Angle       Incremental rotation angle about the Y-axis in radians.

    @retval     None.
**/
VOID
RotateY (float Angle)
{	
    float rotey[4][4];

    SetMem(rotey, sizeof(float)*16, 0);

    // Fill in rotation matrix with y-axis rotation values
    //
    rotey[0][0]=(float)cos_d(Angle);
    rotey[2][0]=(float)-sin_d(Angle);
    rotey[1][1]=1.0;
    rotey[0][2]=(float)sin_d(Angle);
    rotey[2][2]=(float)cos_d(Angle);
    rotey[3][3]=1.0;
	
    // Update the xform matrix
    //
    MatrixMult (CompositeMatrix, rotey);
}


/**
    Applies a 3D scaling rotation about the Z-axis to the current transform matrix.
                                                                 
    @param[in]  Angle       Incremental rotation angle about the Z-axis in radians.

    @retval     None.
**/
VOID
RotateZ (float Angle)
{	
    float rotez[4][4];

    SetMem(rotez, sizeof(float)*16, 0);

    // Fill in rotation matrix with z-axis rotation values
    //
    rotez[0][0]=(float)cos_d(Angle);
    rotez[1][0]=(float)sin_d(Angle);
    rotez[0][1]=(float)-sin_d(Angle);
    rotez[1][1]=(float)cos_d(Angle);
    rotez[2][2]=rotez[3][3]=1.0;
	
    // Update the xform matrix
    //
    MatrixMult (CompositeMatrix, rotez);
}
