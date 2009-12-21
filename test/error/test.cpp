#ifdef DYNAMIC_ERROR_TEST
#include "../error_dynamic/error_dynamic.h"
#endif

#include <error.h>

#include <boost/bind.hpp>

#include <iostream>
#include <iomanip>

#define FILE_TAG 2725FBAF

namespace
{
  const Error::Location LOCATIONS[] = {
    THIS_LINE,
    THIS_LINE,
    THIS_LINE
  };
  
  const Error::CodeType CODES[] = {
    1,
    Error::ModuleCode<'C', 'P', 'L'>::Value,
    Error::ModuleCode<'a', 'b', '#'>::Value
  };

  void Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
  }

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    std::cout << Error::AttributesToString(loc, code, text);
  }
  
  Error GetError()
  {
    return Error(LOCATIONS[2], CODES[2], "Error3");
  }
  
  void TestError(unsigned level, Error::LocationRef loc, Error::CodeType code, const String& /*text*/)
  {
    Test(code == CODES[level], "Nested errors code test", __LINE__);
    Test(loc == LOCATIONS[level], "Nested errors location test", __LINE__);
  }
  
  void CountDepth(unsigned& maxlevel, unsigned level, Error::LocationRef /*loc*/, Error::CodeType /*code*/, const String& /*text*/)
  {
    maxlevel = std::max(maxlevel, level);
  }
  
  unsigned GetDepth(const Error& e)
  {
    unsigned lvl(0);
    e.WalkSuberrors(boost::bind(CountDepth, boost::ref(lvl), _1, _2, _3, _4));
    return lvl + 1;
  }
  
  void ShowError(const Error& e)
  {
    e.WalkSuberrors(ErrOuter);
  }
  
  #define TEST(a) \
    Test(a, # a, __LINE__)
}

int main()
{
  try
  {
    Test(Error::ModuleCode<'0', '0', '0'>::Value == 0x30303000, "ModuleCode#1", __LINE__);
    Test(Error::ModuleCode<'A', 'B', 'C'>::Value == 0x43424100, "ModuleCode#2", __LINE__);
    Error err0;
    TEST(!err0);
    Error err1(LOCATIONS[0], CODES[0], "Error1");
    TEST(err1 == CODES[0]);
    Error err2(LOCATIONS[1], CODES[1], "Error2");
    TEST(err2 == CODES[1]);
    TEST(err2 != err1);
    err1.AddSuberror(err2);
    const Error& err3 = GetError();
    TEST(err3 == CODES[2]);
    err1.AddSuberror(err3);
    TEST(err1.FindSuberror(CODES[0]) == CODES[0]);
    TEST(err1.FindSuberror(CODES[1]) == err2);
    TEST(err1.FindSuberror(100) == 0);
    TEST(!err0);
    TEST(err1);
    err1.WalkSuberrors(TestError);
    const unsigned curDepth(GetDepth(err1));
    TEST(curDepth == 3);
    TEST(GetDepth(err1.AddSuberror(Error())) == curDepth);
    TEST(GetDepth(Error().AddSuberror(err1)) == 1);
    throw err1;
  }
  catch (const Error& e)
  {
    std::cout << "\n\nDisplaying catched error stack" << std::endl;
    ShowError(e);
  }

#ifdef DYNAMIC_ERROR_TEST
  try
  {
    Error errBase(THIS_LINE, Error::ModuleCode<'B', 'I', 'N'>::Value, "Base error from binary");
    errBase.AddSuberror(ReturnErrorByValue());
    errBase.AddSuberror(Error(THIS_LINE, Error::ModuleCode<'I', 'N', 'T'>::Value, "Intermediate error from binary"));
    Error err;
    ReturnErrorByReference(err);
    errBase.AddSuberror(err);
    throw errBase;
  }
  catch (const Error& e)
  {
    std::cout << "Displaying catched mixed error stack" << std::endl;
    ShowError(e);
  }
#endif
}
