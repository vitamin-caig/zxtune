#include <src/io/provider.h>
#include <src/io/error_codes.h>
#include <src/io/providers_parameters.h>

#include <error.h>
#include <progress_callback.h>

#include <iostream>

namespace
{
  const char EXISTING_FILE[] = "Makefile";
  const char NONEXISTING_FILE[] = "non_existing_file";
#ifdef _WIN32
  const char LOCKED_FILE[] = "C:\\pagefile.sys";
#else
  const char LOCKED_FILE[] = "/etc/shadow";
#endif
  const char EMPTY_FILE[] = "empty";
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      std::cerr << Error::ToString(e) << std::endl;
    }
    return e;
  }

  void Test(const Error& res, const String& text, unsigned line)
  {
    std::cout << (res ? "Failed" : "Passed") << " test '" << text << "' at " << line << std::endl;
    ThrowIfError(res);
  }

  bool Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
    return res;
  }
  
  void CheckError(const Error& e, Error::CodeType code, const String& text, unsigned line)
  {
    Test(e == code, text, line);
    if (e != code)
    {
      throw e;
    }
  }

  Error OpenData(const String& name, const Parameters::Accessor& params)
  {
    try
    {
      ZXTune::IO::OpenData(name, params, Log::ProgressCallback::Stub());
      return Error();
    }
    catch (const Error& e)
    {
      return e;
    }
  }
}

int main()
{
  try
  {
    using namespace ZXTune::IO;
    std::cout << "------ test for openers --------\n";
    Parameters::Container::Ptr params = Parameters::Container::Create();
    params->SetValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, std::numeric_limits<int64_t>::max());//set always buffered
    Test(OpenData(EXISTING_FILE, *params), "Opening in buffer mode", __LINE__);
    CheckError(OpenData(NONEXISTING_FILE, *params), ERROR_NOT_OPENED, "Open non-existent in buffer mode", __LINE__);
    CheckError(OpenData(LOCKED_FILE, *params), ERROR_NOT_OPENED, "Open locked in buffer mode", __LINE__);
    params->SetValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, 0);//set always mmaped
    Test(OpenData(EXISTING_FILE, *params), "Opening in mmap mode", __LINE__);
    CheckError(OpenData(NONEXISTING_FILE, *params), ERROR_NOT_OPENED, "Open non-existent in shared mode", __LINE__);  
    CheckError(OpenData(LOCKED_FILE, *params), ERROR_NOT_OPENED, "Open locked in shared mode", __LINE__);
    CheckError(OpenData(EMPTY_FILE, *params), ERROR_NOT_OPENED, "Open empty file", __LINE__);
  }
  catch (const Error& e)
  {
    std::cerr << Error::ToString(e) << std::endl;
    return 1;
  }
}
