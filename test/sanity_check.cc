/*
 * This is an example unit test that doesn't really do anything useful.
 * It is here as a reference for you when creating additional unit tests.
 * For additional reference information, see the "test.h" header.
 */

#include "test.h" // Brings in the UnitTest++ framework

#include "vtkDataManager.h"
#include "vtkSmartPointer.h"
#include "vtkViewRect.h"
#include "vtkDynamicViewFrame.h"
#include "vtkImageViewPane.h"

TEST(sanity_check) // Declares a test named "sanity_check"
{
  CHECK(true); // We certainly hope that true is true
  CHECK_EQUAL(2,1+1); // The value 1+1 should equal 2

  int x[] = {1,2,3};
  int y[] = {1,2,3};
  CHECK_ARRAY_EQUAL(x,y,3); // These arrays of length 3 are equal

  double a = 1.51;
  double b = 1.52;
  CHECK_CLOSE(a,b,0.1); // These equal within 0.1
}

TEST(can_use_view_library) {
  vtkSmartPointer<vtkViewRect> rect =
    vtkSmartPointer<vtkViewRect>::New();
  vtkSmartPointer<vtkDynamicViewFrame> outer_frame =
    vtkSmartPointer<vtkDynamicViewFrame>::New();
  vtkSmartPointer<vtkImageViewPane> pane =
    vtkSmartPointer<vtkImageViewPane>::New();

  rect->SetMainFrame(outer_frame);
  outer_frame->AddChild(pane);

  rect->Start();
}

TEST(can_use_data_library) {
  vtkSmartPointer<vtkDataManager> manager =
    vtkSmartPointer<vtkDataManager>::New();
}
