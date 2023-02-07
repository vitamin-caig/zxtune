/**
 *
 * @file
 *
 * @brief  Error subsystem test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <error.h>

#include <iomanip>
#include <iostream>

#define FILE_TAG 2725FBAF

namespace
{
  // clang-format off
  const Error::Location LOCATIONS[] =
  {
    THIS_LINE,
    THIS_LINE,
    THIS_LINE
  };
  // clang-format on

  const String TEXTS[] = {"Error1", "Error2", "Error3"};

  void Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
    if (!res)
      throw 1;
  }

  Error GetError()
  {
    return Error(LOCATIONS[2], TEXTS[2]);
  }

  void TestSuccess(const Error& err)
  {
    Test(!err, "Success checking test", __LINE__);
    Test(err.GetLocation() == Error::Location(), "Success location test", __LINE__);
    Test(err.GetText() == String(), "Success text test", __LINE__);
  }

  void TestError(unsigned idx, const Error& err)
  {
    Test(err && !!err, "Errors checking test", __LINE__);
    Test(err.GetLocation() == LOCATIONS[idx], "Errors location test", __LINE__);
    Test(err.GetText() == TEXTS[idx], "Error text test", __LINE__);
  }

  void ShowError(const Error& e)
  {
    std::cout << e.ToString();
  }
}  // namespace

int main()
{
  try
  {
    Error err0;
    TestSuccess(err0);
    Error err1(LOCATIONS[0], TEXTS[0]);
    TestError(0, err1);
    Error err2(LOCATIONS[1], TEXTS[1]);
    TestError(1, err2);
    err1.AddSuberror(err2);
    const Error& err3 = GetError();
    TestError(2, err3);
    err1.AddSuberror(err3);
    {
      const Error& sub1 = err1.GetSuberror();
      TestError(1, sub1);
      const Error& sub2 = sub1.GetSuberror();
      TestError(2, sub2);
      const Error& sub3 = sub2.GetSuberror();
      TestSuccess(sub3);
    }
    ThrowIfError(err1);
  }
  catch (const Error& e)
  {
    std::cout << "\n\nDisplaying catched error stack" << std::endl;
    ShowError(e);
  }
  catch (int code)
  {
    return code;
  }
}
