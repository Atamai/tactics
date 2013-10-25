#include "UnitTest++.h"

#include "cbProbe.h"

#include <iostream>

SUITE (TestProbe) {

  TEST (ProbeShouldExist) {
    cbProbe p;
    CHECK(&p);
  }

  TEST (ShouldRoundMembersAtPrint) {
    cbProbe p(1.0, 2.1, 3.2, 4.5, 5.6, -1.4);

    std::string expected("noid |  | x:1 y:2 z:3 | AP:6 | LR:5 | D:-1");
    std::string actual = p.ToString();

    CHECK_EQUAL(0, expected.compare(actual));
  }

  TEST (ShouldInitializeMembers) {
    double p[3] = {1.0, 2.0, 3.0};
    double o[2] = {4.0, 5.0};
    double d = -2.0;

    cbProbe probe(p[0], p[1], p[2], o[0], o[1], d);

    double r[3];
    probe.GetPosition(r);

    double a[3];
    probe.GetOrientation(a);

    double e = probe.GetDepth();

    CHECK_ARRAY_EQUAL(p, r, 3);
    CHECK_ARRAY_EQUAL(o, a, 2);
    CHECK_EQUAL(d, e);
  }

  TEST (ShouldCopyWithConstructor) {
    cbProbe probe(1.0, 2.0, 3.0, 4.0, 5.0, 1.0, "ted");
    cbProbe copy(probe);

    double p[3];
    probe.GetPosition(p);
    double r[3];
    copy.GetPosition(r);

    double o[2];
    probe.GetOrientation(o);
    double a[2];
    copy.GetOrientation(a);

    std::string n1 = probe.GetName();
    std::string n2 = copy.GetName();

    CHECK_ARRAY_EQUAL(p, r, 3);
    CHECK_ARRAY_EQUAL(o, a, 2);
    CHECK_EQUAL(0, n1.compare(n2));
  }

  TEST (ShouldCopyWithAssignment) {
    cbProbe probe(1.0, 2.0, 3.0, 4.0, 5.0, 1.0, "ted");
    cbProbe copy = probe;

    double p[3];
    probe.GetPosition(p);
    double r[3];
    copy.GetPosition(r);

    double o[2];
    probe.GetOrientation(o);
    double a[2];
    copy.GetOrientation(a);

    double  d = probe.GetDepth();
    double  e = copy.GetDepth();

    std::string n1 = probe.GetName();
    std::string n2 = copy.GetName();

    CHECK_ARRAY_EQUAL(p, r, 3);
    CHECK_ARRAY_EQUAL(o, a, 2);
    CHECK_EQUAL(d, e);
    CHECK_EQUAL(0, n1.compare(n2));
  }

  TEST (ShouldReturnString) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, 1.0);
    std::string actual = p.ToString();

    std::string expected("noid |  | x:1 y:2 z:3 | AP:4 | LR:5 | D:1");

    CHECK_EQUAL(0, expected.compare(actual));
  }

  TEST (ShouldAcceptNewPosition) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, 1.0);

    double expected[3] = {1.1, 2.2, 3.3};
    p.SetPosition(expected);

    double actual[3];
    p.GetPosition(actual);

    CHECK_ARRAY_EQUAL(actual, expected, 3);
  }

  TEST (ShouldAcceptNewOrientation) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, 1.0);

    double expected[2] = {5.1, 4.2};
    p.SetOrientation(expected);

    double actual[2];
    p.GetOrientation(actual);

    CHECK_ARRAY_EQUAL(actual, expected, 2);
  }

  TEST (ShouldAllowOutputStreamOperator) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, 1.0);

    std::stringstream stream;
    stream << p;
    std::string actual = stream.str();

    std::string expected("noid  1 2 3 4 5 1");

    CHECK_EQUAL(0, expected.compare(actual));
  }

  TEST (ShouldAcceptName) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, 1.0, "Ted");

    std::string actual = p.ToString();
    std::string expected("Ted |  | x:1 y:2 z:3 | AP:4 | LR:5 | D:1");

    CHECK_EQUAL(0, expected.compare(actual));
  }

  TEST (ShouldSetNewName) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, 1.0, "Ted");
    p.SetName("Bob");

    std::string actual = p.ToString();
    std::string expected("Bob |  | x:1 y:2 z:3 | AP:4 | LR:5 | D:1");

    CHECK_EQUAL(0, expected.compare(actual));
  }

  TEST (ShouldSetNewDepth) {
    cbProbe p(1.0, 2.0, 3.0, 5.0, 4.0, -1.0, "Ted");
    double starting_depth_actual = p.GetDepth();
    double starting_depth_expected = -1.0;

    CHECK_CLOSE(starting_depth_expected, starting_depth_actual, 0.01);

    p.SetDepth(-4.0);
    double depth_actual = p.GetDepth();

    CHECK_CLOSE(-4.0, depth_actual, 0.01);
  }
}
