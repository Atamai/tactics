/*=========================================================================

  Program:   ToolCursor
  Module:    vtkToolCursorInteractorObserver.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkToolCursorInteractorObserver - Observer for vtkToolCursor
// .SECTION Description
// This is a helper class for vtkToolCursor.  Specifically, this class
// receives events from a vtkRenderWindowInteractor and then calls the
// appropriate methods of vtkToolCursor.

#ifndef __vtkToolCursorInteractorObserver_h
#define __vtkToolCursorInteractorObserver_h

#include "vtkInteractorObserver.h"

class vtkToolCursor;

class VTK_EXPORT vtkToolCursorInteractorObserver :
  public vtkInteractorObserver
{
public:
  // Description:
  // Instantiate the object.
  static vtkToolCursorInteractorObserver *New();

  // Description:
  // Standard vtkObject methods
  vtkTypeMacro(vtkToolCursorInteractorObserver,
                       vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable the cursor.
  virtual void SetEnabled(int enable);

  // Description:
  // Set the cursor that this object observes events for.
  void SetToolCursor(vtkToolCursor *cursor);
  vtkToolCursor *GetToolCursor() { return this->ToolCursor; };

  // Description:
  // Get vtkToolCursor "modifier" bits from a VTK keysym.
  static int ModifierFromKeySym(const char *keysym);

protected:
  vtkToolCursorInteractorObserver();
  ~vtkToolCursorInteractorObserver();

  static void ProcessPassiveEvents(vtkObject* object,
                                   unsigned long event,
                                   void* clientdata,
                                   void* calldata);

  static void ProcessEvents(vtkObject* object,
                            unsigned long event,
                            void* clientdata,
                            void* calldata);

  vtkCallbackCommand *PassiveEventCallbackCommand;

  vtkToolCursor *ToolCursor;

private:
  vtkToolCursorInteractorObserver(const vtkToolCursorInteractorObserver&);  //Not implemented
  void operator=(const vtkToolCursorInteractorObserver&);  //Not implemented
};

#endif
