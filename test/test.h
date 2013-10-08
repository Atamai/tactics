/* This file provides some simple unit test macros. */

#include <math.h>
#include <vector>

class UnitTest
{
public:
  UnitTest();
  static int RunAllTests();

protected:
  virtual void operator() () = 0;

  static std::vector<UnitTest *> *Tests;
  static bool TestFailed;

private:
  friend class UnitTestInitializer;
};

static class UnitTestInitializer
{
public:
  UnitTestInitializer();
  ~UnitTestInitializer();
} unitTestSchwarzCounter;

inline UnitTest::UnitTest()
{
  UnitTest::Tests->push_back(this);
}

inline int UnitTest::RunAllTests()
{
  UnitTest::TestFailed = false;
  for (size_t i = 0; i < UnitTest::Tests->size(); i++)
    {
    (*UnitTest::Tests->at(i))();
    }
  return UnitTest::TestFailed;
}

#define CHECK(t) \
if (!(t)) \
{ \
  cout << __FILE__ << ":" << __LINE__ << ": "; \
  cout << "Test assertion failed: " << #t << " [UnitTest]\n"; \
  cout.flush(); \
  UnitTest::TestFailed = true; \
}

#define CHECK_EQUAL(expected, actual) \
CHECK((expected) == (actual))

#define CHECK_ARRAY_EQUAL(x, y, size) \
{ \
  size_t array_size = (size); \
  for (size_t array_index = 0; array_index < array_size; array_index++) \
    { \
    CHECK((x)[array_index] == (y)[array_index]); \
    } \
}

#define CHECK_CLOSE(x, y, tol) \
CHECK(fabs((x) - (y)) < (tol))
 
#define TEST(name) \
class UnitTest_##name : UnitTest \
{ \
  void operator() (); \
} UnitTest_##name##_Instance; \
void UnitTest_##name::operator() ()

#define TEST_MAIN() \
std::vector<UnitTest *> *UnitTest::Tests; \
bool UnitTest::TestFailed; \
static size_t schwarzCounter = 0; \
UnitTestInitializer::UnitTestInitializer() \
{ \
  if (schwarzCounter++ == 0) \
    { \
    UnitTest::Tests = new std::vector<UnitTest *>; \
    } \
} \
UnitTestInitializer::~UnitTestInitializer() \
{ \
  if (--schwarzCounter == 0) \
    { \
    delete UnitTest::Tests; \
    } \
} \
int main(int, char* []) \
{ \
  return UnitTest::RunAllTests(); \
}
