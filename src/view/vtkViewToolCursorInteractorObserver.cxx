/*=========================================================================
  Program: Cerebra
  Module:  vtkViewToolCursorInteractorObserver.cxx

  Copyright (c) 2011-2013 David Adair
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of the Calgary Image Processing and Analysis Centre
    (CIPAC), the University of Calgary, nor the names of any authors nor
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "vtkObjectFactory.h"

#include "vtkCamera.h"
#include "vtkToolCursor.h"
#include "vtkViewToolCursorInteractorObserver.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCallbackCommand.h"

#include "vtkViewRect.h"

//#define DEBUG

vtkStandardNewMacro(vtkViewToolCursorInteractorObserver);

//----------------------------------------------------------------------------
vtkViewToolCursorInteractorObserver::vtkViewToolCursorInteractorObserver()
{
  this->EventCallbackCommand->SetCallback(
    vtkViewToolCursorInteractorObserver::ProcessEvents);
  this->PassiveEventCallbackCommand->SetCallback(
    vtkViewToolCursorInteractorObserver::ProcessPassiveEvents);

  this->FocusCursor = NULL;
  this->ViewRect = NULL;
  this->FocusButton = 0;
}

//----------------------------------------------------------------------------
vtkViewToolCursorInteractorObserver::~vtkViewToolCursorInteractorObserver()
{
  if (this->FocusCursor)
    {
    this->FocusCursor->Delete();
    }
  if (this->ViewRect)
    {
    this->ViewRect->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkViewToolCursorInteractorObserver::ProcessPassiveEvents(
  vtkObject *object, unsigned long event, void *clientdata, void *)
{
  vtkViewToolCursorInteractorObserver* self =
    reinterpret_cast<vtkViewToolCursorInteractorObserver *>(clientdata);

  vtkRenderWindowInteractor *iren = self->GetInteractor();

  vtkToolCursor *cursor = self->GetToolCursor();
  if (!cursor)
    {
    int x, y;
    iren->GetEventPosition(x, y);
    cursor = self->GetViewRect()->RequestToolCursor(x, y);
    if (!cursor)
      {
      // Could not locate a suitable tool cursor
      return;
      }
    }

  // Look for events from the RenderWindow
  vtkRenderWindow *renwin = vtkRenderWindow::SafeDownCast(object);
  if (renwin && iren && renwin->GetInteractor() == iren)
    {
    if (event == vtkCommand::StartEvent)
      {
      // Just before the RenderWindow renders, get the mouse position and
      // use it to set the cursor position.

      int x, y;
      iren->GetEventPosition(x, y);
      cursor->SetDisplayPosition(x, y);
      }
    else if (event == vtkCommand::EndEvent)
      {
      // At end of RenderWindow render, check whether cursor is visible.
      // Hide system cursor if 3D cursor is visible.
      if (cursor->GetVisibility())
        {
        renwin->HideCursor();
        }
      else
        {
        renwin->ShowCursor();
        }
      }
   }

  // All remaining events will be from the interactor
  if (!iren || iren != vtkRenderWindowInteractor::SafeDownCast(object))
    {
    return;
    }

  switch (event)
    {
    case vtkCommand::MouseMoveEvent:
    case vtkCommand::LeftButtonPressEvent:
    case vtkCommand::RightButtonPressEvent:
    case vtkCommand::MiddleButtonPressEvent:
    case vtkCommand::LeftButtonReleaseEvent:
    case vtkCommand::RightButtonReleaseEvent:
    case vtkCommand::MiddleButtonReleaseEvent:
      {
      // First look for modifier keys
      int modifierMask = (VTK_TOOL_SHIFT | VTK_TOOL_CONTROL);
      int modifier = 0;
      if (iren->GetShiftKey()) { modifier |= VTK_TOOL_SHIFT; }
      if (iren->GetControlKey()) { modifier |= VTK_TOOL_CONTROL; }

      // Next look for the button, and whether it was pressed or released
      if (event == vtkCommand::LeftButtonPressEvent)
        {
        modifierMask |= VTK_TOOL_B1;
        modifier |= VTK_TOOL_B1;
        }
      else if (event == vtkCommand::RightButtonPressEvent)
        {
        modifierMask = VTK_TOOL_B2;
        modifier = VTK_TOOL_B2;
        }
      else if (event == vtkCommand::MiddleButtonPressEvent)
        {
        modifierMask = VTK_TOOL_B3;
        modifier = VTK_TOOL_B3;
        }
      else if (event == vtkCommand::LeftButtonReleaseEvent)
        {
        modifierMask |= VTK_TOOL_B1;
        }
      else if (event == vtkCommand::RightButtonReleaseEvent)
        {
        modifierMask = VTK_TOOL_B2;
        }
      else if (event == vtkCommand::MiddleButtonReleaseEvent)
        {
        modifierMask = VTK_TOOL_B3;
        }

      // Set the modifier with the button and key information
      cursor->SetModifierBits(modifier, modifierMask);

      // Need to check if mouse is in the renderer, even if some other
      // observer has focus, so do it here as a passive operation.
      int x, y;
      iren->GetEventPosition(x, y);
      cursor->SetDisplayPosition(x, y);
      }
      break;

    case vtkCommand::EnterEvent:
      {
      // Mouse move events might cease after mouse leaves the window,
      // leaving EventPosition with the last in-window value
      cursor->SetIsInViewport(1);
      }
      break;

    case vtkCommand::LeaveEvent:
      {
      // Mouse move events might cease after mouse leaves the window,
      // leaving EventPosition with the last in-window value
      cursor->SetIsInViewport(0);
      }
      break;

    case vtkCommand::KeyPressEvent:
      {
      // We need to know the exact moment when modifier keys change
      int modifierMask = self->ModifierFromKeySym(iren->GetKeySym());
      int modifier = modifierMask;
      cursor->SetModifierBits(modifier, modifierMask);
      }
      break;

    case vtkCommand::KeyReleaseEvent:
      {
      int modifierMask = self->ModifierFromKeySym(iren->GetKeySym());
      int modifier = 0;
      cursor->SetModifierBits(modifier, modifierMask);
      }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkViewToolCursorInteractorObserver::ProcessEvents(vtkObject *object,
                                                       unsigned long event,
                                                       void *clientdata,
                                                       void *)
{
  vtkViewToolCursorInteractorObserver* self =
    reinterpret_cast<vtkViewToolCursorInteractorObserver *>(clientdata);

  vtkRenderWindowInteractor *iren = self->GetInteractor();
  if (!iren || iren != vtkRenderWindowInteractor::SafeDownCast(object))
    {
    return;
    }

  vtkToolCursor *cursor = self->GetToolCursor();
  if (!cursor)
    {
    int x, y;
    iren->GetEventPosition(x, y);
    cursor = self->GetViewRect()->RequestToolCursor(x, y);
    if (!cursor)
      {
      // Could not locate a suitable tool cursor
      return;
      }
    }

  // Is it safe to grab the focus for the cursor?  Check to see if the
  // InteractorStyle is currently doing an action.
  vtkInteractorStyle *istyle =
    vtkInteractorStyle::SafeDownCast(iren->GetInteractorStyle());
  int allowTakeFocus = (cursor->GetIsInViewport() &&
                        (!istyle || istyle->GetState() == VTKIS_NONE));
  int allowReleaseFocus = (!istyle || istyle->GetState() == VTKIS_NONE);

#ifdef DEBUG
std::cout << "event: " << vtkCommand::GetStringFromEventId(event) << std::endl;
#endif

  switch (event)
    {
    case vtkCommand::MouseMoveEvent:
      {
      int x, y;
      iren->GetEventPosition(x, y);
      self->MoveToDisplayPosition(x, y);
      }
      break;

    case vtkCommand::LeftButtonPressEvent:
    case vtkCommand::RightButtonPressEvent:
    case vtkCommand::MiddleButtonPressEvent:
      {
      int button = 0;
      if (event == vtkCommand::LeftButtonPressEvent)
        {
        button = 1;
        }
      else if (event == vtkCommand::RightButtonPressEvent)
        {
        button = 2;
        }
      else if (event == vtkCommand::MiddleButtonPressEvent)
        {
        button = 3;
        }

      // Need to do ComputePosition to force a pick so that the cursor
      // knows if any focus-worthy object is under the mouse
      int x, y;
      iren->GetEventPosition(x, y);

      cursor->ComputePosition();
      // Only grab focus if there currently is no focus
      if (self->FocusCursor == NULL)
        {
        self->FocusCursor = cursor;
        self->SetFocusButton(button);
        }
      self->MoveToDisplayPosition(x, y);

      // Make sure that no other observers see the event
      self->EventCallbackCommand->SetAbortFlag(1);

      if (allowTakeFocus)
        {
        iren->GetRenderWindow()->SetDesiredUpdateRate(100);
        if (cursor->PressButton(button))
          {
          self->GrabFocus(self->EventCallbackCommand,
                          self->EventCallbackCommand);
          }
        }
      }
      break;

    case vtkCommand::LeftButtonReleaseEvent:
    case vtkCommand::RightButtonReleaseEvent:
    case vtkCommand::MiddleButtonReleaseEvent:
      {
      int button = 0;
      if (event == vtkCommand::LeftButtonReleaseEvent)
        {
        button = 1;
        }
      else if (event == vtkCommand::RightButtonReleaseEvent)
        {
        button = 2;
        }
      else if (event == vtkCommand::MiddleButtonReleaseEvent)
        {
        button = 3;
        }

      int x, y;
      iren->GetEventPosition(x, y);

      // Only lose focus if there currently is a focus and if the button that
      // gave focus was released.
      if (self->FocusCursor && button == self->GetFocusButton())
        {
        self->FocusCursor = NULL;
        }
      self->MoveToDisplayPosition(x, y);

      if (allowReleaseFocus)
        {
        if (cursor->ReleaseButton(button))
          {
          self->ReleaseFocus();
          }
        }
        iren->GetRenderWindow()->SetDesiredUpdateRate(0.0001);
      }
      break;

    //case vtkCommand::EnterEvent: // wait until move occurs before rendering
    case vtkCommand::LeaveEvent:
    case vtkCommand::KeyPressEvent:
    case vtkCommand::KeyReleaseEvent:
      // Just render for these events, they were handled as passive events
      break;
    }

  iren->Render();
}

//----------------------------------------------------------------------------
void vtkViewToolCursorInteractorObserver::MoveToDisplayPosition(double x,
                                                                double y)
{
  vtkToolCursor *focus = this->FocusCursor;

  vtkToolCursor *passive = this->ViewRect->RequestToolCursor(x, y);

  if (focus)
    {
    focus->MoveToDisplayPosition(x, y);

    bool synchronized = true;
    if (synchronized)
      {
      vtkCamera *camera = focus->GetRenderer()->GetActiveCamera();
      std::vector<vtkRenderer *> renderers = this->ViewRect->RequestRenderers();
      for (size_t i = 0; i < renderers.size(); i++)
        {
        if (renderers[i]->GetActiveCamera() != camera)
          {
          renderers[i]->GetActiveCamera()->DeepCopy(camera);
          double focal[3];
          camera->GetFocalPoint(focal);
          renderers[i]->GetActiveCamera()->SetFocalPoint(focal);
          renderers[i]->GetActiveCamera()->Modified();
          }
        }
      }
    }
  if (!focus && passive)
    {
    this->SetToolCursor(passive);
    passive->MoveToDisplayPosition(x, y);
    }

}

//----------------------------------------------------------------------------
void vtkViewToolCursorInteractorObserver::SetViewRect(vtkViewRect *rect)
{
  this->ViewRect = rect;
  rect->Register(this);
  this->SetInteractor(this->ViewRect->GetInteractor());
}
