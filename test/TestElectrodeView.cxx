#include "UnitTest++.h"

#include "vtkDataManager.h"

SUITE (TestElectrodeView) {

  struct DataManagerFixture {
    DataManagerFixture() {
      manager_ = vtkDataManager::New();
    }
    ~DataManagerFixture() {
      manager_->Delete();
    }

    vtkDataManager *manager_;
  };
}
