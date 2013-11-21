#include "UnitTest++.h"
#include "TestData.h"

#include "cbProbeCatalogue.h"
#include "cbProbeSpecification.h"

#include <iostream>
#include <string>

SUITE (TestProbeCatalogue) {
  struct CatalogueFixture {
    CatalogueFixture() : directory_path_(APPLICATION_DATA_PATH), catalogue_(directory_path_) {};
    ~CatalogueFixture() {};

    std::string directory_path_;
    cbProbeCatalogue catalogue_;
  };

  TEST_FIXTURE(CatalogueFixture, TestShouldImportCatalogue) {
    std::vector<std::string> list = catalogue_.list_as_strings();

    std::vector<std::string> expected;
#ifdef TACTICS_PRIVATE_PROBES
    expected.push_back(std::string("3387S-40"));
    expected.push_back(std::string("3389S-40"));
    expected.push_back(std::string("3391S-40"));
    expected.push_back(std::string("6142"));
    expected.push_back(std::string("6146"));
    expected.push_back(std::string("AD04R-SP10X-000"));
    expected.push_back(std::string("AD08R-SP05X-000"));
    expected.push_back(std::string("CIPAC"));
    expected.push_back(std::string("MD01R-SP00X-000"));
    expected.push_back(std::string("PiscesQuad-3487A"));
    expected.push_back(std::string("RD05R-SP35X-000"));
    expected.push_back(std::string("RD06R-SP05X-000"));
    expected.push_back(std::string("RD08R-SP04X-000"));
    expected.push_back(std::string("RD08R-SP05X-000"));
    expected.push_back(std::string("RD10R-SP04X-000"));
    expected.push_back(std::string("RD10R-SP05X-000"));
    expected.push_back(std::string("RD10R-SP06X-000"));
    expected.push_back(std::string("RD10R-SP07X-000"));
    expected.push_back(std::string("RD10R-SP08X-000"));
    expected.push_back(std::string("RD10R-SP35X-000"));
    expected.push_back(std::string("RD12R-SP05X-000"));
    expected.push_back(std::string("RD12R-SP08X-000"));
    expected.push_back(std::string("RD14R-SP03X-000"));
    expected.push_back(std::string("RD14R-SP04X-000"));
    expected.push_back(std::string("RD14R-SP05X-000"));
    expected.push_back(std::string("RD14R-SP06X-000"));
    expected.push_back(std::string("RD14R-SP07X-000"));
    expected.push_back(std::string("RD14R-SP08X-000"));
    expected.push_back(std::string("RD14R-SP09X-000"));
    expected.push_back(std::string("RD14R-SP10X-000"));
    expected.push_back(std::string("RD14R-SP35X-000"));
    expected.push_back(std::string("SD10R-SP05X-000"));
    expected.push_back(std::string("SD12R-SP05X-000"));
#else
    expected.push_back(std::string("CIPAC"));
#endif

    CHECK_EQUAL(expected.size(), list.size());

    for (size_t i = 0; i < list.size(); i++) {
      CHECK_EQUAL(0, expected.at(i).compare(list.at(i)));
    }
  }

  TEST_FIXTURE(CatalogueFixture, TestShouldQueryCatalogue) {
    std::string type("CIPAC");

    cbProbeSpecification spec = catalogue_.specification(type);

    CHECK_EQUAL(0, type.compare(spec.catalogue_number()));
  }
}
