/*=========================================================================
  Program: Cerebra
  Module:  vtkViewRect.cxx

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

#include <assert.h>
#include <queue>

#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkToolCursor.h"
#include "vtkDynamicViewFrame.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkViewFrame.h"
#include "vtkViewObjectCollection.h"
#include "vtkViewPane.h"
#include "vtkViewRect.h"

vtkStandardNewMacro(vtkViewRect);

vtkViewRect::vtkViewRect()
{
  this->RenderWindow = vtkRenderWindow::New();
  this->Interactor = vtkRenderWindowInteractor::New();
  this->RenderWindow->SetInteractor(this->Interactor);
  vtkCallbackCommand *keypressCallback = vtkCallbackCommand::New();
  keypressCallback->SetCallback(vtkViewRect::KeyPressCallbackFunction);
  keypressCallback->SetClientData(this);
  this->Interactor->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);
  this->Frame = NULL;

  this->Tracked = false;

  vtkRenderer *blank = vtkRenderer::New();
  this->RenderWindow->AddRenderer(blank);
  blank->Delete();
}

vtkViewRect::~vtkViewRect()
{
  this->RenderWindow->Delete();
  this->Interactor->Delete();
  if (this->Frame)
    {
    this->Frame->Delete();
    }
}

void vtkViewRect::Start()
{
  this->UpdateRenderers();
  this->Render();
  this->UpdateAllBorders();
  //this->Interactor->Start();
}

void vtkViewRect::SetMainFrame(vtkDynamicViewFrame *frame)
{
  assert(frame != NULL);
  if (this->Frame == NULL)
    {
    this->Frame = frame;
    frame->Register(this);
    }
}

void vtkViewRect::UpdateRenderers()
{
  std::queue<vtkViewObject *> queue;
  queue.push(this->Frame);

  while (!queue.empty())
    {
    vtkViewObject *obj = queue.front();
    queue.pop();

    double currentView[4];
    obj->GetViewport(currentView);

    vtkViewPane *pane = vtkViewPane::SafeDownCast(obj);
    if (pane)
      {
      pane->UpdateViewport(currentView);
      this->AddRenderer(pane->GetRenderer());
      }

    vtkDynamicViewFrame *frame = vtkDynamicViewFrame::SafeDownCast(obj);
    if (frame)
      {
      vtkViewObjectCollection *children = frame->GetChildren();
      int currentChildren = children->GetNumberOfItems();

      for (int i = 0; i < currentChildren; i++)
        {
        vtkViewObject *child = children->GetViewObject(i);
        queue.push(child);
        frame->UpdateChildView(i);
        }
      }
    }
}

void vtkViewRect::AddRenderer(vtkRenderer *ren)
{
  assert(ren != NULL);
  this->RenderWindow->AddRenderer(ren);
}

void vtkViewRect::Render()
{
  assert(this->RenderWindow != NULL);
  this->RenderWindow->Render();
}

vtkToolCursor *vtkViewRect::RequestToolCursor(int x, int y)
{
  int *size = this->RenderWindow->GetSize();
  double xSize = static_cast<double>(size[0]);
  double ySize = static_cast<double>(size[1]);

  double normX = static_cast<double>(x) / xSize;
  double normY = static_cast<double>(y) / ySize;

  std::queue<vtkViewObject *> queue;
  queue.push(this->Frame);

  std::vector<vtkViewPane *> list;

  while (!queue.empty())
    {
    vtkViewObject *obj = queue.front();
    queue.pop();

    vtkViewPane *pane = vtkViewPane::SafeDownCast(obj);
    vtkDynamicViewFrame *frame = vtkDynamicViewFrame::SafeDownCast(obj);
    if (pane)
      {
      list.push_back(pane);
      }
    else if (frame)
      {
      vtkViewObjectCollection *children = frame->GetChildren();
      int currentChildren = children->GetNumberOfItems();

      for (int i = 0; i < currentChildren; i++)
        {
        vtkViewObject *child = children->GetViewObject(i);
        queue.push(child);
        }
      }
    }


  for (size_t i = 0; i < list.size(); i++)
    {
    double v[4];
    list.at(i)->GetViewport(v);
    if (list.at(i)->Contains(normX, normY))
      {
      //temporary solution
      this->Tracked = list.at(i)->GetCursorTracking();
      return list.at(i)->GetToolCursor();      
      }
    }

  if (list.size() > 0)
    {
    //temporary solution
    this->Tracked = list.at(0)->GetCursorTracking();
    return list.at(0)->GetToolCursor();    
    }

  return NULL;
}

std::vector<vtkRenderer *> vtkViewRect::RequestRenderers()
{
  std::vector<vtkRenderer *> renderers;

  std::queue<vtkViewObject *> queue;
  queue.push(this->Frame);

  while (!queue.empty())
    {
    vtkViewObject *obj = queue.front();
    queue.pop();

    vtkViewPane *pane = vtkViewPane::SafeDownCast(obj);
    vtkDynamicViewFrame *frame = vtkDynamicViewFrame::SafeDownCast(obj);
    if (pane)
      {
      renderers.push_back(pane->GetRenderer());
      }
    else if (frame)
      {
      vtkViewObjectCollection *children = frame->GetChildren();
      int currentChildren = children->GetNumberOfItems();

      for (int i = 0; i < currentChildren; i++)
        {
        vtkViewObject *child = children->GetViewObject(i);
        queue.push(child);
        }
      }
    }

  return renderers;
}

void vtkViewRect::KeyPressCallbackFunction(vtkObject *caller,
                                           long unsigned int vtkNotUsed(eventId),
                                           void *clientData,
                                           void *vtkNotUsed(callData))
{
  vtkRenderWindowInteractor *i = static_cast<vtkRenderWindowInteractor *>(caller);
  vtkViewRect *r = static_cast<vtkViewRect *>(clientData);
  char *key = i->GetKeySym();
  switch (*key)
  {
  case 'h':
    r->Frame->SetOrientation(vtkDynamicViewFrame::HORIZONTAL);
    break;
  case 'v':
    r->Frame->SetOrientation(vtkDynamicViewFrame::VERTICAL);
    break;
  case 'g':
    r->Frame->SetOrientation(vtkDynamicViewFrame::GRID);
    break;
  default:
    std::cout << key << std::endl;
  }
  r->UpdateRenderers();
  r->Render();
}

void vtkViewRect::UpdateAllCamerasToMatch(vtkCamera *camera)
{
  std::vector<vtkRenderer *> renderers = this->RequestRenderers();
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

void vtkViewRect::UpdateAllBorders()
{
  std::vector<vtkViewPane *> panes;

  std::queue<vtkViewObject *> queue;
  queue.push(this->Frame);

  while (!queue.empty())
    {
    vtkViewObject *obj = queue.front();
    queue.pop();

    vtkViewPane *pane = vtkViewPane::SafeDownCast(obj);
    vtkDynamicViewFrame *frame = vtkDynamicViewFrame::SafeDownCast(obj);
    if (pane)
      {
      panes.push_back(pane);
      }
    else if (frame)
      {
      vtkViewObjectCollection *children = frame->GetChildren();
      int currentChildren = children->GetNumberOfItems();

      for (int i = 0; i < currentChildren; i++)
        {
        vtkViewObject *child = children->GetViewObject(i);
        queue.push(child);
        }
      }
    }

  for (size_t i = 0; i < panes.size(); i++)
    {
    if (panes[i]->GetBorderEnabled())
      {
      panes[i]->AddBorder();
      }
    }
}
