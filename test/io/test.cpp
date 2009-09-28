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

  const char FILE_URI_NO[] = "?part1/part2\\part3\?part4#part5";

  const char HTTP_URI[] = "http://example.com#subpath";
  const char HTTP_URI_BASE[] = "http://example.com";
  const char HTTP_URI_SUBPATH[] = "subpath";

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
    if (Test(!ZXTune::IO::SplitUri(uri, base, subpath), String("Splitting ") + type, __LINE__))
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
  TestSplitUri(HTTP_URI, HTTP_URI_BASE, HTTP_URI_SUBPATH, "http uri");
  Test(SplitUri(FILE_URI_NO, base, subpath) == NOT_SUPPORTED, "Splitting uri with no base uri", __LINE__);
}
