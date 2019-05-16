/** @file

Implements Ui Progress Circle / Donut.
This supports two modes.  ProgressMode 1 - 100% or manual mode which allows drawing whatever segments
you specify.

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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <UiPrimitiveSupport.h>
#include <Library/UiProgressCircleLib.h>



#define OUTSIDE_CONTROL (0xFF)
#define OUTER_RADIUS (101)
//VALID SEGMENTS VALUES 1 - 100

#define BMP_PADDING 1

typedef struct{
  ProgressCircle PublicPC;
  UINT32 ProgressBackgroundColor;         //Color for unused progress
  UINT32 ProgressSegmentColor;            //Color for used progress
  INT8   ProgressCurrentState;            //Current percentage   0-100
  INT8   ProgressPreviousState;           //Previous percentage  0-100
  POINT  UpperLeft;                       //Upper left corner of circle bounding box in screen coordinates
  INTN   BmpWidth;                        //Width of circle bounding box including required padding.
  UINT8  BitmapData[0];
} PRIVATE_ProgressCircle;

//
// To determine what segment a point is in 
// the slope can be compared.  This table includes
// the slopes for 1-25%.  because a circle can be
// mirrored to all quadrants this covers 100%
//
INTN mSlopeMap[] = {
  (15894),
  (7915),
  (5242),
  (3894),
  (3077),
  (2525),
  (2125),
  (1818),
  (1575),
  (1376),
  (1208),
  (1064),
  (939),
  (827),
  (726),
  (634),
  (549),
  (470),
  (395),
  (324),
  (256),
  (190),
  (126),
  (62),
  (0)
};



/**
Interal function to init all private data members
**/
static
VOID
PRIVATE_Init(IN PRIVATE_ProgressCircle *this);


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
)
{
  if (OuterRadius < InnerRadius)
  {
    ASSERT(OuterRadius > InnerRadius);
    return NULL;
  }

  if (FrameBufferBase == NULL)
  {
    ASSERT(NULL != FrameBufferBase);
    return NULL;
  }

  if (Orgin == NULL)
  {
    ASSERT(NULL != Orgin);
    return NULL;
  }

  //extra allocation is for segment bitmap. 
  PRIVATE_ProgressCircle *this = (PRIVATE_ProgressCircle *)AllocateZeroPool(sizeof(PRIVATE_ProgressCircle) + ((OuterRadius + BMP_PADDING) * (OuterRadius + BMP_PADDING) * 4));
  ASSERT(NULL != this);
  if (this != NULL)
  {
    this->PublicPC.Orgin = *Orgin;
    this->PublicPC.FrameBufferBase = FrameBufferBase;
    this->PublicPC.PixelsPerScanLine = PixelsPerScanLine;
    this->PublicPC.OuterRadius = OuterRadius;
    this->PublicPC.InnerRadius = InnerRadius;

    PRIVATE_Init(this);
    return &this->PublicPC;
  }

  return NULL;
}

/*
Method to free all allocated memory of the ProgressCircle

@param this     - ProgressCircle object to draw

*/
VOID
EFIAPI
delete_ProgressCircle(
IN ProgressCircle *this
)
{
  if (this != NULL){
    FreePool(this);
  }
}

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
)
{
  PRIVATE_ProgressCircle *thispri = (PRIVATE_ProgressCircle*)this;
  if (this == NULL)
  {
    ASSERT(this != NULL);
    return;
  }

  if (thispri->ProgressCurrentState != -1)
  {
    DEBUG((DEBUG_ERROR, "Can't InitializeProgress because progress has already started.\n"));
    return;
  }

  thispri->ProgressBackgroundColor = BgColor;
  thispri->ProgressSegmentColor = ProgressColor;

}

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
)
{
  PRIVATE_ProgressCircle *thispri = (PRIVATE_ProgressCircle*)this;
  if (this == NULL)
  {
    ASSERT(this != NULL);
    return;
  }


  if (Progress < thispri->ProgressCurrentState)
  {
    DEBUG((DEBUG_ERROR, "Can't set requested state (%d) to less than current (%d)\n", Progress, thispri->ProgressCurrentState));
    return;
  }

  if ((Progress < 0) || (Progress > 100))
  {
    DEBUG((DEBUG_ERROR, "Can't set requested state (%d) invalid\n", Progress));
    return;
  }

  if (Progress > thispri->ProgressCurrentState)
  {
    thispri->ProgressPreviousState = thispri->ProgressCurrentState;
    thispri->ProgressCurrentState = Progress;
  }

  if (thispri->ProgressCurrentState == 0)
  {
    DEBUG((DEBUG_VERBOSE, "Drawing Background\n"));
    DrawAll(this, thispri->ProgressBackgroundColor);

    //return early because it was 0 which means no segment drawing
    return; 
  }

  for (INT8 S = (thispri->ProgressPreviousState + 1); S <= thispri->ProgressCurrentState; S++)
  {
    DEBUG((DEBUG_VERBOSE, "Drawing Segment %d\n", S));
    DrawSegment(this, S, thispri->ProgressSegmentColor);
  }
}

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
)
{
  PRIVATE_ProgressCircle *thispri = (PRIVATE_ProgressCircle*)this;
  UINT32 *Pix = NULL;
  UINT8  *cur = NULL;

  if (this == NULL)
  {
    ASSERT(this != NULL);
    return;
  }

  Pix = ((UINT32*)thispri->PublicPC.FrameBufferBase) + (thispri->UpperLeft.Y * thispri->PublicPC.PixelsPerScanLine) + thispri->UpperLeft.X;
  cur =(UINT8*)thispri->BitmapData;
  for (UINT16 Y = 0; Y < thispri->BmpWidth; Y++)
  {
    for (UINT16 X = 0; X < thispri->BmpWidth; X++)
    {
      if (*cur != OUTSIDE_CONTROL)
      {
        *(Pix + X) = Color;
      }
      cur++;
    }
    //increment Pix 1 row
    Pix = Pix + thispri->PublicPC.PixelsPerScanLine;
  }
}


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
)
{
  PRIVATE_ProgressCircle *thispri = (PRIVATE_ProgressCircle*)this;
  UINT32 *Pix = NULL;
  UINT8  *cur = NULL;
  BOOLEAN FoundOnce = FALSE;  //to optimize the exit of loop
  BOOLEAN FoundInThisRow = FALSE; //to optimize the exit of loop

  if (this == NULL)
  {
    ASSERT(this != NULL);
    return;
  }

  if ((Segment < 1) || (Segment > 100))
  {
    DEBUG((DEBUG_ERROR, "Segment Invalid: %d\n", Segment));
    return;
  }

  //Find start of circle bmp in framebuffer space. 
  Pix = ((UINT32*)thispri->PublicPC.FrameBufferBase) + (thispri->UpperLeft.Y * thispri->PublicPC.PixelsPerScanLine) + thispri->UpperLeft.X;

  //Get pointer to start of circle bmp
  cur = (UINT8*)thispri->BitmapData;

  //iterate each line looking for requested segment
  for (UINT16 Y = 0; Y < thispri->BmpWidth; Y++)
  {
    FoundInThisRow = FALSE;  //reset for next row

    for (UINT16 X = 0; X < thispri->BmpWidth; X++)
    {
      if (*cur == Segment)
      {
        *(Pix + X) = Color;
        FoundInThisRow = TRUE;
        FoundOnce = TRUE;
      }
      cur++;
    }

    //exit early 
    if (FoundOnce && !FoundInThisRow)
    {
      break; 
    }

    //increment Pix 1 row
    Pix = Pix + thispri->PublicPC.PixelsPerScanLine;

  }
}



//---------------------------------------------------------------------------------------
// PRIVATE FUNCTIONS
//---------------------------------------------------------------------------------------

/**
Internal function to find a start and end point of a given horizontal line and then fill
each point between them with given value. 
**/
static
VOID
Fill(IN  PRIVATE_ProgressCircle *this, UINT8 Value)
{

  //go line by line vertically
  for (UINT16 Y = 0; Y < this->BmpWidth; Y++)
  {
    UINT8* start = NULL;
    UINT8  *cur = this->BitmapData + (Y * this->BmpWidth);
    //go across a line horizontally starting on the left side
    for (UINT16 X = 0; X < this->BmpWidth; X++)
    {
      //find outer edge
      if (*cur == OUTER_RADIUS) 
      {
        //find the left side
        if (start == NULL)
        {
          start = cur;
        }
        else  //right side
        {
          //fill between left and right side
          //use odd do while to avoid memset
          do 
          {
            *start++ = Value;  //set and increment
          } while (start <= cur);
        }
      } //if condition for outer radius detection
      cur++;
    }  //X loop
  } // y loop
}

/**
Internal function used to find the segment of a given point. 
Segment between 1-100 returned.
This routine mirrors the point into known quadrant, then calculates
the slope and then compares with slope list to find which segment it is in.  
Finally the segment is adjusted based on the quadrant of the original point.  
**/
static
UINT8
FindSegment(IN PRIVATE_ProgressCircle *this, POINT a)
{
  POINT t = a;
  UINT8 Seg;
  INTN Slope;
  INTN BmpOrgin = this->BmpWidth / 2;

  //first convert into first Quadrant
  if (t.X < BmpOrgin)
  {
    t.X = (BmpOrgin -t.X) + BmpOrgin;
  }

  if (t.Y > BmpOrgin)
  {
    t.Y = BmpOrgin - (t.Y - BmpOrgin);
  }

  //Catch special cases where rise/run calc doesn't work
  if (t.X == BmpOrgin)
  {
    Slope = mSlopeMap[0] + 1;
  }
  else if (t.Y == BmpOrgin)
  {
    Slope = mSlopeMap[24] + 1;
  }
  else
  {
    //compute it
    Slope = ((BmpOrgin - t.Y) * 1000) / (t.X - BmpOrgin);  //1000 x slope value (integer math trick)
  }
  Seg = 0;
  while (mSlopeMap[Seg++] > Slope); 

  ASSERT(Slope >= 0);
  ASSERT(Seg <= 25);
  ASSERT(Seg > 0);

  //now we know our segment.  Now just need to adjust for quad
  if (t.Y != a.Y)
  {
    Seg = (50 - Seg) + 1;
  }
  if (t.X != a.X)
  {
    Seg = 100 - Seg + 1;
  }

  return Seg;
}


/**
Private function iterate thru all points and
each point inside the donut will have its Segment 
determined. 

**/
static
VOID
Segmatize(
IN  PRIVATE_ProgressCircle *this)
{
  UINT8  *cur = this->BitmapData;
  for (UINT16 Y = 0; Y < this->BmpWidth; Y++)
  {
    for (UINT16 X = 0; X < this->BmpWidth; X++)
    {
      if (*cur != OUTSIDE_CONTROL) 
      {
        POINT t;
        t.X = X;
        t.Y = Y;
        *cur = FindSegment(this, t);
      }
      cur++;
    } //for x
  } //for y
}

/**
Private function to mark one pixel in the Bitmap Data
**/
VOID
SetPixel(
IN PRIVATE_ProgressCircle *this,
IN INTN X,
IN INTN Y,
IN UINT8 Value
)
{
  UINT8 *temp = this->BitmapData;
  temp += ((Y * this->BmpWidth) + X);
  *temp = Value;
}


/**
Private function supporting drawing a circle.
This will mark all points in the private bitmap with a given value.  
This uses the symetry of the circle to draw all points.  
This also translates from circle coordinates (-radius, radius) to bitmap coordinates (0, BmpWidth)

**/
VOID
MarkAllPoints(
IN PRIVATE_ProgressCircle *this,
IN POINT P,
IN UINT8 Value
)
{
  INTN bmpcenter = (this->BmpWidth / 2);
  if (P.X == 0)
  {
    //at vertical point
    SetPixel(this, bmpcenter, bmpcenter + P.Y, Value);  //Q1
    SetPixel(this, bmpcenter + P.Y, bmpcenter, Value);  //Q2
    SetPixel(this, bmpcenter, bmpcenter - P.Y, Value);  //Q3
    SetPixel(this, bmpcenter - P.Y, bmpcenter, Value);  //Q4
    
  }
  else if (P.X == P.Y)
  {
    //at 45deg point
    SetPixel(this, bmpcenter + P.X, bmpcenter + P.Y, Value);  //Q1
    SetPixel(this, bmpcenter + P.X, bmpcenter - P.Y, Value);  //Q2
    SetPixel(this, bmpcenter - P.X, bmpcenter - P.Y, Value);  //Q3
    SetPixel(this, bmpcenter - P.X, bmpcenter + P.Y, Value);  //Q4
  }
  else if (P.X < P.Y)
  {
    //from 0 < angle < 45  --mirror 8 times
    SetPixel(this, bmpcenter + P.X, bmpcenter + P.Y, Value);   //Q1.1
    SetPixel(this, bmpcenter + P.Y, bmpcenter + P.X, Value);   //Q1.2

    SetPixel(this, bmpcenter + P.X, bmpcenter - P.Y, Value);   //Q2.1
    SetPixel(this, bmpcenter + P.Y, bmpcenter - P.X, Value);   //Q2.2

    SetPixel(this, bmpcenter - P.X, bmpcenter - P.Y, Value);   //Q3.1
    SetPixel(this, bmpcenter - P.Y, bmpcenter - P.X, Value);   //Q3.2

    SetPixel(this, bmpcenter - P.X, bmpcenter + P.Y, Value);   //Q4.1
    SetPixel(this, bmpcenter - P.Y, bmpcenter + P.X, Value);   //Q4.2
  }
  else
  {
    DEBUG((DEBUG_ERROR, "Shouldn't get here.  Point is (%d, %d)\n", P.X, P.Y));
  }
}

/**
Private function to draw a single circle radius using 
the midpoint algorithm adjusted for integers

**/
static
VOID
DrawCircleEdgeUsingMidPointAlg(
IN PRIVATE_ProgressCircle *this,
IN INTN RadiusToDraw,
IN UINT8 MarkValue 
)
{
  POINT c;
  c.X = 0;
  c.Y = RadiusToDraw;
  INTN Mid = 1 - c.Y;
  do {
    MarkAllPoints(this, c, MarkValue);
    c.X++;
    if (Mid <= 0)
    {
      Mid += 2 * c.X + 1;
    }
    else
    {
      c.Y--;
      Mid += 2 * (c.X - c.Y) + 1;
    }
  } while (c.X <= c.Y);
  
  return;
}


/**
  Private function to Init all internal members 
  and figure out all information needed for drawing and segments


**/
static
VOID
PRIVATE_Init(IN PRIVATE_ProgressCircle *this)
{
  //find bounding box start
  this->UpperLeft.X = (this->PublicPC.Orgin.X - this->PublicPC.OuterRadius-BMP_PADDING);
  this->UpperLeft.Y = (this->PublicPC.Orgin.Y - this->PublicPC.OuterRadius - BMP_PADDING);
  this->BmpWidth = (this->PublicPC.OuterRadius + BMP_PADDING) *2;
  this->ProgressCurrentState = -1;
  this->ProgressPreviousState = 0;
  this->ProgressBackgroundColor = 0xFFFFFFFF;
  this->ProgressSegmentColor = 0x00000000;

  DEBUG((DEBUG_INFO, "BmpWidth %d Orgin: %d\n", this->BmpWidth, (this->BmpWidth / 2)));

  SetMem(this->BitmapData, (this->BmpWidth * this->BmpWidth), OUTSIDE_CONTROL);  //init bitmap to all nothing

  //figure out donut and segments
  //1. Draw Outer Radius
  DrawCircleEdgeUsingMidPointAlg(this, this->PublicPC.OuterRadius, OUTER_RADIUS);
  Fill(this, 8);
  DrawCircleEdgeUsingMidPointAlg(this, this->PublicPC.InnerRadius, OUTER_RADIUS);
  Fill(this, OUTSIDE_CONTROL);  //remove the middel
  Segmatize(this);  //break into 100 segments
}
