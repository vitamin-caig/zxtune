#include <src/io/provider.h>
#include <src/io/error_codes.h>

#include <error.h>

#include <iostream>
#include <iomanip>

namespace
{
  const char EXISTING_FILE[] = "Makefile";
  const char NONEXISTING_FILE[] = "non_existing_file";
  const char LOCKED_FILE[] = "/etc/shadow";

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2$#x\n\tAt: %3%\n\t--------\n") % text % code % Error::LocationToString(loc)).str();
    std::cout << txt;
  }
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      e.WalkSuberrors(ErrOuter);
    }
    return e;
  }

  bool Test(const Error& res, const String& text, unsigned line)
  {
    std::cout << (res ? "Failed" : "Passed") << " test '" << text << "' at " << line << std::endl;
    ShowIfError(res);
    return !res;
  }

  bool Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
    return res;
  }
}

int main()
{
  using namespace ZXTune::IO;
  std::cout << "------ test for openers --------\n";
  String subpath;
  OpenDataParameters params;
  DataContainer::Ptr data;
  Test(OpenData(EXISTING_FILE, params, data, subpath), "Opening in buffer mode", __LINE__);
  Test(ShowIfError(OpenData(NONEXISTING_FILE, params, data, subpath)), "Open non-existent in buffer mode", __LINE__);
  Test(ShowIfError(OpenData(LOCKED_FILE, params, data, subpath)), "Open locked in buffer mode", __LINE__);
  params.Flags = USE_MMAP;
  Test(OpenData(EXISTING_FILE, params, data, subpath), "Opening in mmap mode", __LINE__);
  Test(ShowIfError(OpenData(NONEXISTING_FILE, params, data, subpath)), "Open non-existent in shared mode", __LINE__);  
  Test(ShowIfError(OpenData(LOCKED_FILE, params, data, subpath)), "Open locked in shared mode", __LINE__);
}
