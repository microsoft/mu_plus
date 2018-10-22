/** @file

  Implements the base "class" definition for all control objects.

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

#ifndef _UIT_BASE_H_
#define _UIT_BASE_H_


//////////////////////////////////////////////////////////////////////////////
// Control Base Class Definition
//

typedef OBJECT_STATE
    (* DrawFunctionPtr)(IN     void                *this,
             IN     BOOLEAN             DrawHighlight,
             IN     SWM_INPUT_STATE     *pInputState,
             OUT    VOID                **pSelectionContext);
typedef EFI_STATUS
    (* SetControlBoundsFunctionPtr)(IN void        *this,
                                    IN SWM_RECT    Bounds);

typedef EFI_STATUS
    (* GetControlBoundsFunctionPtr)(IN  void       *this,
                                    OUT SWM_RECT   *Bounds);

typedef EFI_STATUS
    (* SetControlStateFunctionPtr)(IN VOID            *this,
                                   IN OBJECT_STATE    State);

typedef OBJECT_STATE
    (*GetControlStateFunctionPtr)(IN  void       *this);

typedef EFI_STATUS
    (* CopySettingsFunctionPtr)(IN void        *this,
                                IN void        *prev);

typedef struct _ControlBase
{
    // *** Member variables ***
    //
    OBJECT_TYPE        ControlType;

    // *** Functions ***
    //
    VOID
    (*Dtor)(IN VOID     *this);

    DrawFunctionPtr Draw;
    SetControlBoundsFunctionPtr SetControlBounds;
    GetControlBoundsFunctionPtr GetControlBounds;
    SetControlStateFunctionPtr SetControlState;
    GetControlStateFunctionPtr GetControlState;
    CopySettingsFunctionPtr CopySettings;


} ControlBase;

//////////////////////////////////////////////////////////////////////////////
// Public
//



#endif  // _UIT_BASE_H_.
