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

  void Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
  }

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("%1%\n\nCode: %2%\nAt: %3%\n--------\n") % text % code % Error::LocationToString(loc)).str();
    std::cout << txt;
  }
  
  Error GetError()
  {
    return Error(LOCATIONS[2], 3, "Error3");
  }
  
  void TestError(unsigned level, Error::LocationRef loc, Error::CodeType code, const String& /*text*/)
  {
    Test(level == code - 1, "Nested errors code test", __LINE__);
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
    Test(Error::ModuleCode<'0', '0'>::Value == 0x30300000, "ModuleCode#1", __LINE__);
    Test(Error::ModuleCode<'A', 'B'>::Value == 0x42410000, "ModuleCode#2", __LINE__);
    Error err0;
    TEST(!err0);
    Error err1(LOCATIONS[0], 1, "Error1");
    TEST(err1 == 1);
    Error err2(LOCATIONS[1], 2, "Error2");
    TEST(err2 == 2);
    TEST(err2 != err1);
    err1.AddSuberror(err2);
    const Error& err3 = GetError();
    TEST(err3 == 3);
    err1.AddSuberror(err3);
    TEST(err1.FindSuberror(1) == 1);
    TEST(err1.FindSuberror(2) == err2);
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
    Error errBase(THIS_LINE, Error::ModuleCode<'b', 'i'>::Value, "Base error from binary");
    errBase.AddSuberror(ReturnErrorByValue());
    errBase.AddSuberror(Error(THIS_LINE, Error::ModuleCode<'i', 'n'>::Value, "Intermediate error from binary"));
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
