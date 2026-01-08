/*=========================================================================
  Program: Cerebra
  Module:  qvtkViewToolCursorWidget.cxx

  Copyright (c) 2011-2013 Qian Lu, David Gobbi, David Adair
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

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#endif

#include "qvtkViewToolCursorWidget.h"

#include "vtkToolCursor.h"
#include "vtkViewRect.h"
#include "vtkDynamicViewFrame.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

#include "qpaintengine.h"

#include "qdebug.h"
#include "qevent.h"
#include "qapplication.h"
#include "qpainter.h"
#include "qsignalmapper.h"
#include "qtimer.h"
#if defined(Q_WS_X11)
#include "qx11info_x11.h"
#endif

//#include "vtkstd/map"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindow.h"
#include "vtkCommand.h"
#include "vtkOStrStreamWrapper.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
//#include "vtkConfigure.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include <iostream>

// function to dirty cache when a render occurs.
static void dirty_cache(vtkObject *, unsigned long, void *, void *);

/*! constructor */
qvtkViewToolCursorWidget::qvtkViewToolCursorWidget(QWidget* p, Qt::WindowFlags f)
: QWidget(p, f | Qt::MSWindowsOwnDC), mRenWin(NULL),
    cachedImageCleanFlag(false),
    automaticImageCache(false), maxImageCacheRenderRate(1.0)

{
  this->synchronized = false;
  this->LayoutSwitching = false;
  // translucent background
  this->setAttribute(Qt::WA_TranslucentBackground);
  // no double buffering
  this->setAttribute(Qt::WA_PaintOnScreen);

  // default to strong focus
  this->setFocusPolicy(Qt::StrongFocus);

  // default to enable mouse events when a mouse button isn't down
  // so we can send enter/leave events to VTK
  this->setMouseTracking(true);

  // set expanding to take up space for better default layouts
  this->setSizePolicy(
      QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding )
      );

  mPaintEngine = NULL;

  this->mCachedImage = vtkImageData::New();
  this->mCachedImage->SetOrigin(0,0,0);
  this->mCachedImage->SetSpacing(1,1,1);
  this->mCachedImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  this->FocusCursor = NULL;
  this->FocusCursorShape.setShape(Qt::ArrowCursor);
  this->setCursor(this->FocusCursorShape);
  this->FocusButton = 0;
  this->WheelDelta = 0;
}

/*! destructor */

qvtkViewToolCursorWidget::~qvtkViewToolCursorWidget()
{
  // get rid of the VTK window
  this->SetRenderWindow(NULL);

  this->mCachedImage->Delete();

  if (mPaintEngine)
  {
    delete mPaintEngine;
  }
}

/*! get the render window
*/
vtkRenderWindow* qvtkViewToolCursorWidget::GetRenderWindow()
{
  if (!this->mRenWin)
    {
    // create a default vtk window
    vtkRenderWindow* win = vtkRenderWindow::New();
    this->SetRenderWindow(win);
    win->Delete();
    }

  return this->mRenWin;
}

/*! set the render window
  this will bind a VTK window with the Qt window
  it'll also replace an existing VTK window
  */
void qvtkViewToolCursorWidget::SetRenderWindow(vtkRenderWindow* w)
{
  // do nothing if we don't have to
  if (w == this->mRenWin)
  {
    return;
  }

  // unregister previous window
  if (this->mRenWin)
  {
    //clean up window as one could remap it
    if (this->mRenWin->GetMapped())
    {
      this->mRenWin->Finalize();
    }
#ifdef Q_WS_X11
    this->mRenWin->SetDisplayId(NULL);
#endif
    this->mRenWin->SetWindowId(NULL);
    this->mRenWin->UnRegister(NULL);
  }

  // now set the window
  this->mRenWin = w;

  if (this->mRenWin)
  {
    // register new window
    this->mRenWin->Register(NULL);

    // if it is mapped somewhere else, unmap it
    if (this->mRenWin->GetMapped())
    {
      this->mRenWin->Finalize();
    }

#ifdef Q_WS_X11
    // give the qt display id to the vtk window
    this->mRenWin->SetDisplayId(QX11Info::display());
#endif

    // special x11 setup
    x11_setup_window();

    // give the qt window id to the vtk window
    this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));

    // tell the vtk window what the size of this window is
    this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());
    this->mRenWin->vtkRenderWindow::SetPosition(this->x(), this->y());

    // have VTK start this window and create the necessary graphics resources
    if (isVisible())
    {
      this->mRenWin->Start();
    }

    this->mRenWin->SetSize(this->width(), this->height());

    // Add an observer to monitor when the image changes.  Should work most
    // of the time.  The application will have to call
    // markCachedImageAsDirty for any other case.
    vtkCallbackCommand *cbc = vtkCallbackCommand::New();
    cbc->SetClientData(this);
    cbc->SetCallback(dirty_cache);
    this->mRenWin->AddObserver(vtkCommand::RenderEvent, cbc);
    cbc->Delete();
  }
}

void qvtkViewToolCursorWidget::markCachedImageAsDirty()
{
  if (this->cachedImageCleanFlag)
  {
    this->cachedImageCleanFlag = false;
    emit cachedImageDirty();
  }
}

void qvtkViewToolCursorWidget::saveImageToCache()
{
  if (this->cachedImageCleanFlag)
  {
    return;
  }

  int w = this->width();
  int h = this->height();
  this->mCachedImage->SetExtent(0, w-1, 0, h-1, 0, 0);
  this->mCachedImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray* array = vtkUnsignedCharArray::SafeDownCast(
      this->mCachedImage->GetPointData()->GetScalars());
  // We use back-buffer if
  this->mRenWin->GetPixelData(0, 0, this->width()-1, this->height()-1,
                              this->mRenWin->GetDoubleBuffer()? 0 /*back*/ : 1 /*front*/, array);
  this->cachedImageCleanFlag = true;
  emit cachedImageClean();
}

void qvtkViewToolCursorWidget::setAutomaticImageCacheEnabled(bool flag)
{
  this->automaticImageCache = flag;
  if (!flag)
  {
    this->mCachedImage->Initialize();
    this->mCachedImage->SetOrigin(0,0,0);
    this->mCachedImage->SetSpacing(1,1,1);
    this->mCachedImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  }
}

bool qvtkViewToolCursorWidget::isAutomaticImageCacheEnabled() const
{
  return this->automaticImageCache;
}

void qvtkViewToolCursorWidget::setMaxRenderRateForImageCache(double rate)
{
  this->maxImageCacheRenderRate = rate;
}

double qvtkViewToolCursorWidget::maxRenderRateForImageCache() const
{
  return this->maxImageCacheRenderRate;
}

vtkImageData* qvtkViewToolCursorWidget::cachedImage()
{
  // Make sure image is up to date.
  this->paintEvent(NULL);
  this->saveImageToCache();

  return this->mCachedImage;
}

/*! overloaded Qt's event handler to capture additional keys that Qt has
  default behavior for (for example the Tab and Shift-Tab key)
  */
bool qvtkViewToolCursorWidget::event(QEvent* e)
{
  if (e->type() == QEvent::ParentAboutToChange)
  {
    this->markCachedImageAsDirty();
    if (this->mRenWin)
    {
      // Finalize the window to remove graphics resources associated with
      // this window
      if (this->mRenWin->GetMapped())
      {
        this->mRenWin->Finalize();
      }
    }
  }
  else if (e->type() == QEvent::ParentChange)
  {
    if (this->mRenWin)
    {
      x11_setup_window();
      // connect to new window
      this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));

      // start up the window to create graphics resources for this window
      if (isVisible())
      {
        this->mRenWin->Start();
      }
    }
  }

  if (QObject::event(e))
  {
    return 1;
  }

  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent* ke = static_cast<QKeyEvent*>(e);
    this->keyPressEvent(ke);
    return ke->isAccepted();
  }

  return QWidget::event(e);
}


/*! handle resize event
*/
void qvtkViewToolCursorWidget::resizeEvent(QResizeEvent* e)
{
  QWidget::resizeEvent(e);

  if (!this->mRenWin)
  {
    return;
  }

  // Don't set size on subclass of vtkRenderWindow or it triggers recursion.
  // Getting this event in the first place means the window was already
  // resized and we're updating the sizes in VTK.
  this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());

  // ToolCursor does not use resize events yet

  this->markCachedImageAsDirty();
}

void qvtkViewToolCursorWidget::moveEvent(QMoveEvent* e)
{
  QWidget::moveEvent(e);

  if (!this->mRenWin)
  {
    return;
  }

  // This event is for when the window moves, not when the mouse moves

  // Don't set size on subclass of vtkRenderWindow or it triggers recursion.
  // Getting this event in the first place means the window was already
  // resized and we're updating the sizes in VTK.
  this->mRenWin->vtkRenderWindow::SetPosition(this->x(), this->y());

  // ToolCursor does nothing when the widget moves
}

/*! handle paint event
*/
void qvtkViewToolCursorWidget::paintEvent(QPaintEvent* )
{
  // if we have a saved image, use it
  if (this->paintCachedImage())
  {
    return;
  }
  this->mRenWin->vtkRenderWindow::Render();
}
/*! handle mouse press event
*/
void qvtkViewToolCursorWidget::mousePressEvent(QMouseEvent* e)
{
  // If there is already a focus, return
  if (this->FocusCursor)
    {
    return;
    }

  qreal scale = this->devicePixelRatioF();  // retina scaling
  const QPointF pos = e->position();
  int xpos = qRound(pos.x() * scale);
  int ypos = qRound((this->height() - 1 - pos.y()) * scale);
  this->FocusCursor = this->ViewRect->RequestToolCursor(xpos, ypos);

  // If a tool cursor was not found, return
  if (!this->FocusCursor)
    {
    return;
    }

  this->FocusCursor->SetDisplayPosition(xpos, ypos);

  int button = 0;
  int modifier = 0;
  int modifierMask = (VTK_TOOL_SHIFT | VTK_TOOL_CONTROL);
  switch(e->button())
    {
    case Qt::LeftButton:
      modifier |= VTK_TOOL_B1;
      modifierMask |= VTK_TOOL_B1;
      button = 1;
      break;
    case Qt::RightButton:
      modifier |= VTK_TOOL_B2;
      modifierMask |= VTK_TOOL_B2;
      button = 2;
      break;
    case Qt::MiddleButton:
      modifier |= VTK_TOOL_B2;
      modifierMask |= VTK_TOOL_B2;
      button = 3;
      break;
    default:
      break;
    }

  // Set the focus button to track holding down buttons
  this->FocusButton = e->button();

  this->FocusCursor->SetModifierBits(modifier, modifierMask);
  this->FocusCursor->ComputePosition();
  this->FocusCursor->PressButton(button);

  this->ViewRect->Render();
}

/*! handle mouse release event
*/
void qvtkViewToolCursorWidget::mouseReleaseEvent(QMouseEvent* e)
{
  this->ViewRect->GetRenderWindow()->SetDesiredUpdateRate(20);

  // If there is a focus and the button is no longer held down
  if (this->FocusCursor && e->button() == this->FocusButton)
    {
    qreal scale = this->devicePixelRatioF();  // retina scaling
    const QPointF pos = e->position();
    int xpos = qRound(pos.x() * scale);
    int ypos = qRound((this->height() - 1 - pos.y()) * scale);
    this->FocusCursor->SetDisplayPosition(xpos, ypos);
    int button = 0;
    int modifier = 0;
    int modifierMask = (VTK_TOOL_SHIFT | VTK_TOOL_CONTROL);
    switch(e->button())
      {
      case Qt::LeftButton:
        modifierMask |= VTK_TOOL_B1;
        button = 1;
        break;
      case Qt::RightButton:
        modifierMask |= VTK_TOOL_B2;
        button = 2;
        break;
      case Qt::MiddleButton:
        modifierMask |= VTK_TOOL_B2;
        button = 3;
        break;
      default:
        break;
      }

    this->FocusCursor->SetModifierBits(modifier, modifierMask);
    this->FocusCursor->ReleaseButton(button);
    this->FocusCursor = NULL;
    this->ViewRect->Render();
    }
}

/*! handle mouse move event
*/
void qvtkViewToolCursorWidget::mouseMoveEvent(QMouseEvent* e)
{
  this->ViewRect->GetRenderWindow()->SetDesiredUpdateRate(100);

  qreal scale = this->devicePixelRatioF();  // retina scaling
  const QPointF pos = e->position();
  int xpos = qRound(pos.x() * scale);
  int ypos = qRound((this->height() - 1 - pos.y()) * scale);
  if (this->FocusCursor ||
      this->ViewRect->RequestToolCursor(xpos, ypos))
    {
    if(this->ViewRect->GetCursorTracking())
      {
      this->setCursor(this->FocusCursorShape);
      }
    else
      {
      this->setCursor(Qt::ArrowCursor);
      }
    }
  this->MoveToDisplayPosition(xpos, ypos);
  this->ViewRect->Render();
}


/*! handle enter event
*/
void qvtkViewToolCursorWidget::enterEvent(QEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
    {
    // mouse is entering the widget
    this->FocusCursor->SetIsInViewport(1);
    this->mRenWin->vtkRenderWindow::Render();
    }
}

/*! handle leave event
*/
void qvtkViewToolCursorWidget::leaveEvent(QEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
    {
    // mouse is leaving the widget
    this->FocusCursor->SetIsInViewport(0);
    }
}

/*! handle key press event
*/
void qvtkViewToolCursorWidget::keyPressEvent(QKeyEvent* e)
{
  // If the user does not want to be able to switch layouts, return
  if (!this->LayoutSwitching) {
    return;
  }

  switch (e->key())
    {
    case Qt::Key_H:
      this->ViewRect->GetFrame()->SetOrientation(vtkDynamicViewFrame::HORIZONTAL);
      break;
    case Qt::Key_V:
      this->ViewRect->GetFrame()->SetOrientation(vtkDynamicViewFrame::VERTICAL);
      break;
    case Qt::Key_G:
      this->ViewRect->GetFrame()->SetOrientation(vtkDynamicViewFrame::GRID);
      break;
    default:
      std::cout << e->key() << std::endl;
    }
  this->ViewRect->UpdateRenderers();
  this->ViewRect->Render();
}

/*! handle key release event
*/
void qvtkViewToolCursorWidget::keyReleaseEvent(QKeyEvent* vtkNotUsed(e))
{
}

void qvtkViewToolCursorWidget::wheelEvent(QWheelEvent* e)
{
  int modifier = 0;
  int modifierMask = (VTK_TOOL_SHIFT | VTK_TOOL_CONTROL);
  int button = 0;

  qreal scale = this->devicePixelRatioF();  // retina scaling
  const QPointF pos = e->position();
  int xpos = qRound(pos.x() * scale);
  int ypos = qRound((this->height() - 1 - pos.y()) * scale);
  
  int delta = e->angleDelta().y();

  this->WheelDelta += delta;

  if (this->WheelDelta >= 120) {
    if (delta >= 120) {
      this->WheelDelta = 0;
    }
    else {
      this->WheelDelta -= 120;
    }
    modifier = VTK_TOOL_WHEEL_FWD;
    button = 4;
  }
  else if (this->WheelDelta <= -120) {
    if (delta <= -120) {
      this->WheelDelta = 0;
    }
    else {
      this->WheelDelta += 120;
    }
    modifier = VTK_TOOL_WHEEL_BWD;
    button = 5;
  }

  if (modifier) {
    vtkToolCursor *active = this->ViewRect->RequestToolCursor(xpos, ypos);
    active->SetModifierBits(modifier, modifierMask);
    active->ComputePosition();

    // Invoke the event
    active->PressButton(button);

    // Make sure to "release" the scroll
    active->ReleaseButton(button);
  }

  this->ViewRect->Render();
}

void qvtkViewToolCursorWidget::focusInEvent(QFocusEvent* vtkNotUsed(e))
{
  // These prevent updates when the window
  // gains or loses focus.  By default, Qt
  // does an update because the color group's
  // active status changes.  We don't even use
  // color groups so we do nothing here.

  // focus is for key events, not supported by ToolCursor yet
}

void qvtkViewToolCursorWidget::focusOutEvent(QFocusEvent* vtkNotUsed(e))
{
  // These prevent updates when the window
  // gains or loses focus.  By default, Qt
  // does an update because the color group's
  // active status changes.  We don't even use
  // color groups so we do nothing here.

  // focus is for key events, not supported by ToolCursor yet
}


void qvtkViewToolCursorWidget::contextMenuEvent(QContextMenuEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
  {
    // not supported yet
  }
}

void qvtkViewToolCursorWidget::dragEnterEvent(QDragEnterEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
  {
    // not supported yet
  }
}

void qvtkViewToolCursorWidget::dragMoveEvent(QDragMoveEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
  {
    // not supported yet
  }
}

void qvtkViewToolCursorWidget::dragLeaveEvent(QDragLeaveEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
  {
    // not supported yet
  }
}

void qvtkViewToolCursorWidget::dropEvent(QDropEvent* vtkNotUsed(e))
{
  if (this->FocusCursor)
  {
    // not supported yet
  }
}

void qvtkViewToolCursorWidget::showEvent(QShowEvent* e)
{
  this->markCachedImageAsDirty();

  QWidget::showEvent(e);
}

QPaintEngine* qvtkViewToolCursorWidget::paintEngine() const
{
  return mPaintEngine;
}


// X11 stuff near the bottom of the file
// to prevent namespace collisions with Qt headers

#if defined Q_WS_X11
#if defined(VTK_USE_OPENGL_LIBRARY)
#include "vtkXOpenGLRenderWindow.h"
#endif
#ifdef VTK_USE_MANGLED_MESA
#include "vtkXMesaRenderWindow.h"
#endif
#endif

void qvtkViewToolCursorWidget::x11_setup_window()
{
#if defined Q_WS_X11

  // this whole function is to allow this window to have a
  // different colormap and visual than the rest of the Qt application
  // this is very important if Qt's default visual and colormap is
  // not enough to get a decent graphics window


  // save widget states
  bool tracking = this->hasMouseTracking();
  Qt::FocusPolicy focus_policy = focusPolicy();
  bool visible = isVisible();
  if (visible)
  {
    hide();
  }


  // get visual and colormap from VTK
  XVisualInfo* vi = 0;
  Colormap cmap = 0;
  Display* display = reinterpret_cast<Display*>(mRenWin->GetGenericDisplayId());

  // check ogl and mesa and get information we need to create a decent window
#if defined(VTK_USE_OPENGL_LIBRARY)
  vtkXOpenGLRenderWindow* ogl_win = vtkXOpenGLRenderWindow::SafeDownCast(mRenWin);
  if (ogl_win)
  {
    vi = ogl_win->GetDesiredVisualInfo();
    cmap = ogl_win->GetDesiredColormap();
  }
#endif
#ifdef VTK_USE_MANGLED_MESA
  if (!vi)
  {
    vtkXMesaRenderWindow* mgl_win = vtkXMesaRenderWindow::SafeDownCast(mRenWin);
    if (mgl_win)
    {
      vi = mgl_win->GetDesiredVisualInfo();
      cmap = mgl_win->GetDesiredColormap();
    }
  }
#endif

  // can't get visual, oh well.
  // continue with Qt's default visual as it usually works
  if (!vi)
  {
    if (visible)
    {
      show();
    }
    return;
  }

  // create the X window based on information VTK gave us
  XSetWindowAttributes attrib;
  attrib.colormap = cmap;
  attrib.border_pixel = 0;
  attrib.background_pixel = 0;

  Window p = RootWindow(display, DefaultScreen(display));
  if (parentWidget())
  {
    p = parentWidget()->winId();
  }

  XWindowAttributes a;
  XGetWindowAttributes(display, this->winId(), &a);

  Window win = XCreateWindow(display, p, a.x, a.y, a.width, a.height,
                             0, vi->depth, InputOutput, vi->visual,
                             CWBackPixel|CWBorderPixel|CWColormap, &attrib);

  // backup colormap stuff
  Window *cmw;
  Window *cmwret;
  int count;
  if ( XGetWMColormapWindows(display, topLevelWidget()->winId(), &cmwret, &count) )
  {
    cmw = new Window[count+1];
    memcpy( (char *)cmw, (char *)cmwret, sizeof(Window)*count );
    XFree( (char *)cmwret );
    int i;
    for ( i=0; i<count; i++ )
    {
      if ( cmw[i] == winId() )
      {
        cmw[i] = win;
        break;
      }
    }
    if ( i >= count )
    {
      cmw[count++] = win;
    }
  }
  else
  {
    count = 1;
    cmw = new Window[count];
    cmw[0] = win;
  }


  // tell Qt to initialize anything it needs to for this window
  create(win);

  // restore colormaps
  XSetWMColormapWindows( display, topLevelWidget()->winId(), cmw, count );

  delete [] cmw;
  XFree(vi);

  XFlush(display);

  // restore widget states
  this->setMouseTracking(tracking);
  this->setAttribute(Qt::WA_NoBackground);
  this->setAttribute(Qt::WA_PaintOnScreen);
  this->setFocusPolicy(focus_policy);
  if (visible)
  {
    show();
  }

#endif
}

//-----------------------------------------------------------------------------
bool qvtkViewToolCursorWidget::paintCachedImage()
{
  // if we have a saved image, use it
  if (this->cachedImageCleanFlag)
    {
    vtkUnsignedCharArray* array = vtkUnsignedCharArray::SafeDownCast(
        this->mCachedImage->GetPointData()->GetScalars());
    // put cached image into back buffer if we can
    this->mRenWin->SetPixelData(0, 0, this->width()-1, this->height()-1,
                                array, !this->mRenWin->GetDoubleBuffer());
    // swap buffers, if double buffering
    this->mRenWin->Frame();
    // or should we just put it on the front buffer?
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
static void dirty_cache(vtkObject *caller, unsigned long,
                        void *clientdata, void *)
{
  qvtkViewToolCursorWidget *widget = reinterpret_cast<qvtkViewToolCursorWidget *>(clientdata);
  widget->markCachedImageAsDirty();

  vtkRenderWindow *renwin = vtkRenderWindow::SafeDownCast(caller);
  if (renwin)
    {
    if (   widget->isAutomaticImageCacheEnabled()
        && (  renwin->GetDesiredUpdateRate()
            < widget->maxRenderRateForImageCache() ) )
      {
      widget->saveImageToCache();
      }
    }
}

void qvtkViewToolCursorWidget::MoveToDisplayPosition(double xp, double yp)
{
  vtkToolCursor *focus = this->FocusCursor;
  vtkToolCursor *passive = this->ViewRect->RequestToolCursor(xp, yp);

  if (focus)
    {
    focus->MoveToDisplayPosition(xp, yp);
    if (this->synchronized)
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
    passive->SetDisplayPosition(xp, yp);
    }
}
