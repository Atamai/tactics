#include "UnitTest++.h"

#include "cbProbeSpecification.h"

#include <iostream>

SUITE (TestProbeSpecification) {

  struct SpecFixture {
    SpecFixture() {
      default_path_ = "/Users/dadair/CAIN/Brain/Application/Electrode/Probes/6142.txt";
      s_ = new cbProbeSpecification(default_path_);
    }
    ~SpecFixture() {
      delete s_;
    }

    cbProbeSpecification *s_;
    std::string default_path_;
  };

  TEST_FIXTURE(SpecFixture, ShouldImportDefaultData) {
    std::vector<double> points = s_->points();
    std::string catalogue_number = s_->catalogue_number();
    bool tip_is_contact = s_->tip_is_contact();

    std::vector<double> expected_points;
    expected_points.push_back(0.0);
    expected_points.push_back(3.0);
    expected_points.push_back(4.5);
    expected_points.push_back(6.0);
    expected_points.push_back(7.5);
    expected_points.push_back(9.0);
    expected_points.push_back(10.5);
    expected_points.push_back(12.0);
    expected_points.push_back(250.2);
    expected_points.push_back(264.7);

    std::string expected_catalogue_number = "6142";
    bool expected_tip_is_contact = true;

    CHECK_EQUAL(expected_tip_is_contact, tip_is_contact);
    CHECK_EQUAL(0, expected_catalogue_number.compare(catalogue_number));

    CHECK_EQUAL(expected_points.size(), points.size());
    for (size_t i = 0; i < points.size(); i++) {
      CHECK_EQUAL(expected_points[i], points[i]);
    }
  }
}
