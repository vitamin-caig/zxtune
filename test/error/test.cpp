#include "../error_dynamic/error_dynamic.h"

#include <error.h>

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
  
  void ShowError(const Error& e)
  {
    e.WalkSuberrors(ErrOuter);
  }
}

int main()
{
  try
  {
    Test(Error::ModuleCode<'0', '0'>::Value == 0x30300000, "ModuleCode#1", __LINE__);
    Test(Error::ModuleCode<'A', 'B'>::Value == 0x42410000, "ModuleCode#2", __LINE__);
    Error err0;
    Test(!err0, "!err0", __LINE__);
    Error err1(LOCATIONS[0], 1, "Error1");
    Test(err1 == 1,"Error1 == 1", __LINE__);
    Error err2(LOCATIONS[1], 2, "Error2");
    Test(err2 == 2, "Error2 == 2", __LINE__);
    Test(err2 != err1, "err2 != err1", __LINE__);
    err1.AddSuberror(err2);
    const Error& err3 = GetError();
    Test(err3 == 3, "err3 == 3", __LINE__);
    err1.AddSuberror(err3);
    Test(err1.FindSuberror(1) == 1, "err1.FindSuberror(1) == 1", __LINE__);
    Test(err1.FindSuberror(2) == err2, "err1.FindSuberror(2) == err2", __LINE__);
    Test(err1.FindSuberror(100) == 0, "err1.FindSuberror(0) == 0", __LINE__);
    Test(!err0, "!err0", __LINE__);
    Test(err1, "err1", __LINE__);
    err1.WalkSuberrors(TestError);
    throw err1;
  }
  catch (const Error& e)
  {
    std::cout << "Displaying catched error stack" << std::endl;
    ShowError(e);
  }
  
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
}
