#include <src/io/provider.h>
#include <src/io/error_codes.h>

#include <error.h>

#include <iostream>
#include <iomanip>

namespace
{
  const char INVALID_URI[] = "uri://invalid";

  const char FILE_URI_EMPTY[] = "file";
  const char EMPTY[] = {0};

  const char FILE_URI1[] = "C:\\disk\\file\?part1/part2\\part3\?part4#part5";
  const char FILE_URI1_BASE[] = "C:\\disk\\file";
  const char FILE_URI_SUBPATH[] = "part1/part2\\part3\?part4#part5";
  
  const char FILE_URI2[] = "/posix/path/file?part1/part2\\part3\?part4#part5";
  const char FILE_URI2_BASE[] = "/posix/path/file";
  
  const char FILE_URI3[] = "absolute path?part1/part2\\part3\?part4#part5";
  const char FILE_URI3_BASE[] = "absolute path";

  const char FILE_URI4[] = "../../path?part1/part2\\part3\?part4#part5";
  const char FILE_URI4_BASE[] = "../../path";

  const char FILE_URI5[] = "file:///path/from/root?part1/part2\\part3\?part4#part5";
  const char FILE_URI5_BASE[] = "/path/from/root";

  const char FILE_URI_NO[] = "?part1/part2\\part3\?part4#part5";

  const char HTTP_URI[] = "http://example.com#subpath";
  const char HTTP_URI_BASE[] = "http://example.com";
  const char HTTP_URI_SUBPATH[] = "subpath";

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
  
  void OutProvider(const ZXTune::IO::ProviderInfo& info)
  {
    std::cout << 
      "Provider: " << info.Name << std::endl <<
      "Description: " << info.Description << std::endl;
  }
  
  void TestSplitUri(const String& uri, const String& baseEq, const String& subEq, const String& type)
  {
    String base, subpath;
    if (Test(ZXTune::IO::SplitUri(uri, base, subpath), String("Splitting ") + type + ": " + uri, __LINE__))
    {
      Test(base == baseEq, " testing base", __LINE__);
      Test(subpath == subEq, " testing subpath", __LINE__);
    }
  }
}

int main()
{
  using namespace ZXTune::IO;
  std::vector<ProviderInfo> providers;
  GetSupportedProviders(providers);
  std::for_each(providers.begin(), providers.end(), OutProvider);
  String base, subpath;
  Test(SplitUri(INVALID_URI, base, subpath) == NOT_SUPPORTED, "Splitting invalid uri", __LINE__);
  TestSplitUri(FILE_URI_EMPTY, FILE_URI_EMPTY, EMPTY, "uri without subpath");
  TestSplitUri(FILE_URI1, FILE_URI1_BASE, FILE_URI_SUBPATH, "windows file uri");
  TestSplitUri(FILE_URI2, FILE_URI2_BASE, FILE_URI_SUBPATH, "posix file uri");
  TestSplitUri(FILE_URI3, FILE_URI3_BASE, FILE_URI_SUBPATH, "neutral file uri");
  TestSplitUri(FILE_URI4, FILE_URI4_BASE, FILE_URI_SUBPATH, "complex file uri");
  TestSplitUri(FILE_URI5, FILE_URI5_BASE, FILE_URI_SUBPATH, "file uri with scheme");
  TestSplitUri(HTTP_URI, HTTP_URI_BASE, HTTP_URI_SUBPATH, "http uri");
  Test(SplitUri(FILE_URI_NO, base, subpath) == NOT_SUPPORTED, "Splitting uri with no base uri", __LINE__);
  
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
