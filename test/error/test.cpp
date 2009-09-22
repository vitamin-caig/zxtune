#include <error.h>

#include <iostream>
#include <iomanip>

#define FILE_TAG 2725FBAF

namespace
{
  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::Code code, const String& text)
  {
    const String txt = (Formatter("%1%\n\nCode: %2%\nAt: %3%\n--------\n") % text % code % Error::LocationToString(loc)).str();
    std::cout << txt;
  }
  
  void Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
  }
  
  Error GetError()
  {
    return Error(THIS_LINE, 3, "Error3");
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
  Error err0;
  Error err1(THIS_LINE, 1, "Error1");
  Test(err1 == 1,"Error1 == 1", __LINE__);
  Error err2(THIS_LINE, 2, "Error2");
  Test(err2 == 2, "Error2 == 2", __LINE__);
  Test(err2 != err1, "err2 != err1", __LINE__);
  err1.AddSuberror(err2);
  const Error& err3 = GetError();
  Test(err3 == 3, "err3 == 3", __LINE__);
  err1.AddSuberror(err3);
  Test(err1.FindSuberror(1) == 1, "err1.FindSuberror(1) == 1", __LINE__);
  Test(err1.FindSuberror(2) == err2, "err1.FindSuberror(2) == err2", __LINE__);
  Test(!err0, "!err0", __LINE__);
  Test(err1, "err1", __LINE__);
    throw err1;
  }
  catch (const Error& e)
  {
    ShowError(e);
  }
}
