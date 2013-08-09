/*=========================================================================
  Program: Cerebra
  Module:  qvtkViewToolCursorWidget.h

  Copyright (c) 2011-2013 Qian Lu, David Adair
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

#ifndef QVTKVIEWTOOLCURSORWIDGET_H
#define QVTKVIEWTOOLCURSORWIDGET_H

#include <QtGui/QWidget>

#include <vtkConfigure.h>
#include <vtkToolkits.h>

class vtkRenderWindow;
class vtkImageData;
class vtkToolCursor;
class vtkViewRect;

#if defined(Q_WS_MAC)
# if defined(QT_MAC_USE_COCOA) && defined(VTK_USE_COCOA)
#  define QVTK_USE_COCOA
# elif !defined(QT_MAC_USE_COCOA) && defined(VTK_USE_CARBON)
#  define QVTK_USE_CARBON
# elif defined(VTK_USE_COCOA)
#  error "VTK configured to use Cocoa, but Qt configured to use Carbon"
# elif defined(VTK_USE_CARBON)
#  error "VTK configured to use Carbon, but Qt configured to use Cocoa"
# endif
#endif


#if defined(QVTK_USE_CARBON)
#include <Carbon/Carbon.h>    // Event handling for dirty region
#endif

#include "QVTKWin32Header.h"

//! qvtkViewToolCursorWidget displays a VTK window in a Qt window.
class QVTK_EXPORT qvtkViewToolCursorWidget : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(bool automaticImageCacheEnabled
             READ isAutomaticImageCacheEnabled
             WRITE setAutomaticImageCacheEnabled)
  Q_PROPERTY(double maxRenderRateForImageCache
             READ maxRenderRateForImageCache
             WRITE setMaxRenderRateForImageCache)

public:
  //! constructor
  qvtkViewToolCursorWidget(QWidget* parent = NULL, Qt::WFlags f = 0);
  //! destructor
  virtual ~qvtkViewToolCursorWidget();

  // Description:
  // Set the vtk render window, if you wish to use your own vtkRenderWindow
  virtual void SetRenderWindow(vtkRenderWindow*);

  void SetSynchronized(bool s) { this->synchronized = s; }
  void SetLayoutSwitching(bool s) { this->LayoutSwitching = s; }

  // Description:
  // Set the cursor shape for actived viewFrame
  void SetFocusCursorShape(QCursor cur) { this->FocusCursorShape = cur; }
  QCursor GetFocusCursorShape() const { return this->FocusCursorShape; }

  // Description:
  // Get the vtk render window.
  virtual vtkRenderWindow* GetRenderWindow();

  // Description:
  // Enables/disables automatic image caching.  If disabled (the default),
  // qvtkViewToolCursorWidget will not call saveImageToCache() on its own.
  virtual void setAutomaticImageCacheEnabled(bool flag);
  virtual bool isAutomaticImageCacheEnabled() const;

  // Description:
  // If automatic image caching is enabled, then the image will be cached
  // after every render with a DesiredUpdateRate that is greater than
  // this parameter.  By default, the vtkRenderWindowInteractor will
  // change the desired render rate depending on the user's
  // interactions. (See vtkRenderWindow::DesiredUpdateRate,
  // vtkRenderWindowInteractor::DesiredUpdateRate and
  // vtkRenderWindowInteractor::StillUpdateRate for more details.)
  virtual void setMaxRenderRateForImageCache(double rate);
  virtual double maxRenderRateForImageCache() const;

  // Description:
  // Returns the current image in the window.  If the image cache is up
  // to date, that is returned to avoid grabbing other windows.
  virtual vtkImageData* cachedImage();

  // Description:
  // Handle showing of the Widget
  virtual void showEvent(QShowEvent*);

  // Description:
  // Get the paint engine specific to this class
  virtual QPaintEngine* paintEngine() const;

Q_SIGNALS:
  // Description:
  // This signal will be emitted whenever a mouse event occurs
  // within the QVTK window
  void mouseEvent(QMouseEvent* event);

  // Description:
  // This signal will be emitted whenever the cached image goes from clean
  // to dirty.
  void cachedImageDirty();

  // Description:
  // This signal will be emitted whenever the cached image is refreshed.
  void cachedImageClean();

public Q_SLOTS:
  // Description:
  // This will mark the cached image as dirty.  This slot is automatically
  // invoked whenever the render window has a render event or the widget is
  // resized.  Your application should invoke this slot whenever the image in
  // the render window is changed by some other means.  If the image goes
  // from clean to dirty, the cachedImageDirty() signal is emitted.
  void markCachedImageAsDirty();

  // Description:
  // If the cached image is dirty, it is updated with the current image in
  // the render window and the cachedImageClean() signal is emitted.
  void saveImageToCache();

  void SetViewRect(vtkViewRect *viewRect) { this->ViewRect = viewRect; }
  void MoveToDisplayPosition(double xp, double yp);

protected:
  // overloaded resize handler
  virtual void resizeEvent(QResizeEvent* event);
  // overloaded move handler
  virtual void moveEvent(QMoveEvent* event);
  // overloaded paint handler
  virtual void paintEvent(QPaintEvent* event);

  // overloaded mouse press handler
  virtual void mousePressEvent(QMouseEvent* event);
  // overloaded mouse move handler
  virtual void mouseMoveEvent(QMouseEvent* event);
  // overloaded mouse release handler
  virtual void mouseReleaseEvent(QMouseEvent* event);
  // overloaded key press handler
  virtual void keyPressEvent(QKeyEvent* event);
  // overloaded key release handler
  virtual void keyReleaseEvent(QKeyEvent* event);
  // overloaded enter event
  virtual void enterEvent(QEvent*);
  // overloaded leave event
  virtual void leaveEvent(QEvent*);
#ifndef QT_NO_WHEELEVENT
  // overload wheel mouse event
  virtual void wheelEvent(QWheelEvent*);
#endif
  // overload focus event
  virtual void focusInEvent(QFocusEvent*);
  // overload focus event
  virtual void focusOutEvent(QFocusEvent*);
  // overload Qt's event() to capture more keys
  bool event(QEvent* e);

  // overload context menu event
  virtual void contextMenuEvent(QContextMenuEvent*);
  // overload drag enter event
  virtual void dragEnterEvent(QDragEnterEvent*);
  // overload drag move event
  virtual void dragMoveEvent(QDragMoveEvent*);
  // overload drag leave event
  virtual void dragLeaveEvent(QDragLeaveEvent*);
  // overload drop event
  virtual void dropEvent(QDropEvent*);

  // method called in paintEvent() to render the image cache on to the device.
  // return false, if cache couldn;t be used for painting. In that case, the
  // paintEvent() method will continue with the default painting code.
  virtual bool paintCachedImage();

  // the vtk render window
  vtkRenderWindow* mRenWin;

  // the paint engine
  QPaintEngine* mPaintEngine;

  // set up an X11 window based on a visual and colormap
  // that VTK chooses
  void x11_setup_window();

protected:

  vtkImageData* mCachedImage;
  bool cachedImageCleanFlag;
  bool automaticImageCache;
  double maxImageCacheRenderRate;

  QCursor FocusCursorShape;
  vtkToolCursor *FocusCursor;
  vtkViewRect *ViewRect;
  int FocusButton;
  bool synchronized;
  bool LayoutSwitching;

private:
  //! unimplemented operator=
  qvtkViewToolCursorWidget const& operator=(qvtkViewToolCursorWidget const&);
  //! unimplemented copy
  qvtkViewToolCursorWidget(const qvtkViewToolCursorWidget&);
};

#endif
