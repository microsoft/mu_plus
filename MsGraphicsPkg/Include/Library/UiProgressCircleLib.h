/** @file

Ui Progress Circle / Donut.  
This supports two modes.  ProgressMode 1 - 100% or manual mode which allows drawing whatever segments
you specify.  

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PROGRESS_CIRCLE
#define PROGRESS_CIRCLE

//
// ProgressCircle context
//
typedef struct
{
  POINT                        Orgin;  
  UINT8*                       FrameBufferBase;
  UINTN                        PixelsPerScanLine;
  UINT16                       OuterRadius;
  UINT16                       InnerRadius;
} ProgressCircle;


/*
Method to use create a new ProgressCircle struct.
This structure is used by all the other functions to update and 
draw the progress circle to the screen

@param Orgin             - Center point of progress circle in framebuffer coordinates
@param FrameBufferBase   - pointer to framebuffer address of 0,0  (upper left)
@param PixelsPerScanLine - Number of pixels per scan line.  
                           This is to support aligned framebuffers
@param InnerRadius       - The InnerRadius of the progress circle / donut
@param OuterRadius       - The OuterRadius of the progress circle / donut. 
                           Because of pixel alignment (pixel/integer math) the 
                           radius can deviate from the alignment by 1 pixel at times.
@ret   A new ProgressCircle structure used for updating and drawing the progress circle

*/
ProgressCircle*
EFIAPI
new_ProgressCircle(
  IN POINT *Orgin,
  IN UINT8 *FrameBufferBase,
  IN UINTN PixelsPerScanLine,
  IN UINT16 InnerRadius,
  IN UINT16 OuterRadius
  );


/*
Method to free all allocated memory of the ProgressCircle 

@param this     - ProgressCircle object to draw

*/
VOID 
EFIAPI
delete_ProgressCircle(
  IN ProgressCircle *this
  );

/*
Method to use Init ProgressCircle as a Progress indicator.
This means it will go from 0 - 100 filling in with segment color
as it progresses.

@param this     - ProgressCircle object to draw
@param BgColor  - Color value to fill indicating unused progress
@param ProgressColor - Color to fill indicating used progress

*/
VOID
EFIAPI
InitializeProgress(
IN ProgressCircle *this,
IN UINT32 BgColor,
IN UINT32 ProgressColor
);


/*
Method to use ProgressCircle as a Progress indicator.
This means it will go from 0 - 100 filling in with segment color
as it progresses.  

@param this     - ProgressCircle object to draw
@param Progress - Progress value 0 - 100.  0 = Init with BG color.
                  all other values will progress forward filling as they go.

*/
VOID 
EFIAPI
UpdateProgress(
  IN ProgressCircle *this,
  IN INT8 Progress
  );


/*
Method to draw/fill the entire progress circle.

@param this     - ProgressCircle object to draw
@param Color    - Color value to draw

*/
VOID
EFIAPI
DrawAll(
  IN  ProgressCircle *this, 
  UINT32 Color
  );

/*
  Method to draw/fill a single segment. 
  
  @param this     - ProgressCircle object to draw
  @param Segment  - Segment to draw (1 - 100). 
  @param Color    - Color value to draw segment

*/
VOID
EFIAPI
DrawSegment(
  IN ProgressCircle *this, 
  INT8 Segment,
  UINT32 Color
  );


#endif
