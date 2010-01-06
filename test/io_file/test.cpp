#include <src/io/provider.h>
#include <src/io/error_codes.h>
#include <src/io/providers_parameters.h>

#include <error.h>

#include <iostream>
#include <iomanip>

namespace
{
  const char EXISTING_FILE[] = "Makefile";
  const char NONEXISTING_FILE[] = "non_existing_file";
#ifdef _WIN32
  const char LOCKED_FILE[] = "C:\\pagefile.sys";
#else
  const char LOCKED_FILE[] = "/etc/shadow";
#endif
  
  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2%\n\tAt: %3%\n\t--------\n") % text % Error::CodeToString(code) % Error::LocationToString(loc)).str();
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
  
  void CheckError(const Error& e, Error::CodeType code, const String& text, unsigned line)
  {
    Test(e == code, text, line);
    ShowIfError(e);
  }
}

int main()
{
  using namespace ZXTune::IO;
  std::cout << "------ test for openers --------\n";
  String subpath;
  Parameters::Map params;
  DataContainer::Ptr data;
  params[Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD] = std::numeric_limits<int64_t>::max();//set always buffered
  Test(OpenData(EXISTING_FILE, params, ProgressCallback(), data, subpath), "Opening in buffer mode", __LINE__);
  CheckError(OpenData(NONEXISTING_FILE, params, ProgressCallback(), data, subpath), NOT_OPENED, "Open non-existent in buffer mode", __LINE__);
  CheckError(OpenData(LOCKED_FILE, params, ProgressCallback(), data, subpath), NOT_OPENED, "Open locked in buffer mode", __LINE__);
  params[Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD] = 0;//set always mmaped
  Test(OpenData(EXISTING_FILE, params, ProgressCallback(), data, subpath), "Opening in mmap mode", __LINE__);
  CheckError(OpenData(NONEXISTING_FILE, params, ProgressCallback(), data, subpath), NOT_OPENED, "Open non-existent in shared mode", __LINE__);  
  CheckError(OpenData(LOCKED_FILE, params, ProgressCallback(), data, subpath), NOT_OPENED, "Open locked in shared mode", __LINE__);
}
