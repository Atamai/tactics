/*=========================================================================
  Program: Cerebra
  Module:  vtkViewToolCursorInteractorObserver.h

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

#ifndef __vtkViewToolCursorInteractorObserver_h
#define __vtkViewToolCursorInteractorObserver_h

#include "vtkToolCursorInteractorObserver.h"

class vtkToolCursor;
class vtkViewRect;

//! An observer to render window interactor events.
/*!
 *  This class receives events from the render window interactor and
 *  passes the events through to the render window's appropriate tool cursors.
*/
class vtkViewToolCursorInteractorObserver :
    public vtkToolCursorInteractorObserver
{
public:
 //! A way to intantiate a vtkViewToolCursorInteractorObserver.
  static vtkViewToolCursorInteractorObserver *New();

  //! Standard VTK macro.
  vtkTypeMacro(vtkViewToolCursorInteractorObserver,
               vtkToolCursorInteractorObserver);

  //! Provides a pointer to the outer vtkViewRect to the observer.
  /*!
   *  [B]MUST[/B] be provided in order for the class to function.
   *  \param rect The vtkViewRect.
  */
  void SetViewRect(vtkViewRect *rect);

protected:
  vtkViewToolCursorInteractorObserver();
  ~vtkViewToolCursorInteractorObserver();

  //! Static callback function to process passive events from the interactor.
  /*!
   *  \param object The render window for the scene.
   *  \param event The event id.
   *  \param clientdata Pointer to the observer instance ('this' pointer).
   *  \param calldata Unused.
  */
  static void ProcessPassiveEvents(vtkObject* object,
                                   unsigned long event,
                                   void* clientdata,
                                   void* calldata);

  //! Static callback function to process active events from the interactor.
  /*!
   *  \param object The render window for the scene.
   *  \param event The event id.
   *  \param clientdata Pointer to the observer instance ('this' pointer).
   *  \param calldata Unused.
  */
  static void ProcessEvents(vtkObject* object,
                            unsigned long event,
                            void* clientdata,
                            void* calldata);

  //! Determines which tool to use for the scene.
  /*!
   *  The observer maintains two tool cursors. One for keeping track of
   *  active events, and one for keeping track of passive events. This
   *  method determines which to use.
   *  \param x The x coordinate of the cursor (display coords).
   *  \param y The y coordinate of the cursor (display coords).
  */
  void MoveToDisplayPosition(double x, double y);

  //! Simple get method to retrieve the vtkViewRect.
  vtkViewRect *GetViewRect() { return this->ViewRect; }

  //! Sets the input mouse button ID as the focus button.
  void SetFocusButton(int button) { this->FocusButton = button; }

  //! Returns the mouse button currently with focus.
  int GetFocusButton() const { return this->FocusButton; }

  //! Pointer to which vtkToolCursor currently has focus.
  vtkToolCursor *FocusCursor;

  //! Pointer to the vtkViewRect.
  vtkViewRect *ViewRect;

  //! Variable to track the mouse button being held down.
  int FocusButton;

private:
  vtkViewToolCursorInteractorObserver(const vtkViewToolCursorInteractorObserver&);  //Not implemented
  void operator=(const vtkViewToolCursorInteractorObserver&);  //Not implemented
};

#endif
