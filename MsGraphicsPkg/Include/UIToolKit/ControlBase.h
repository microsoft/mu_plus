/** @file

  Implements the base "class" definition for all control objects.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
