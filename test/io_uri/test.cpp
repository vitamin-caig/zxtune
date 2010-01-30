#include <formatter.h>
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
  
  void OutProvider(const ZXTune::IO::ProviderInformation& info)
  {
    std::cout << 
      "Provider: " << info.Name << std::endl <<
      "Description: " << info.Description << std::endl <<
      "Version: " << info.Version << std::endl;
  }
  
  void TestSplitUri(const String& uri, const String& baseEq, const String& subEq, const String& type)
  {
    String base, subpath;
    if (Test(ZXTune::IO::SplitUri(uri, base, subpath), String("Splitting ") + type, __LINE__))
    {
      Test(base == baseEq, " testing base", __LINE__);
      Test(subpath == subEq, " testing subpath", __LINE__);
      String result;
      if (Test(ZXTune::IO::CombineUri(base, subpath, result), String("Combining ") + type, __LINE__))
      {
        Test(String::npos != uri.find(result), String(" testing result"), __LINE__);
      }
      else
      {
        std::cout << "Skipped result test" << std::endl;
      }
    }
    else
    {
      std::cout << "Skipped result test" << std::endl;
      std::cout << "Skipped combining test" << std::endl;
    }
  }
}

int main()
{
  using namespace ZXTune::IO;
  std::cout << "------ test for enumeration -------\n";
  ProviderInformationArray providers;
  EnumerateProviders(providers);
  std::for_each(providers.begin(), providers.end(), OutProvider);
  String base, subpath;
  std::cout << "------ test for splitters/combiners ------\n";
  Test(SplitUri(INVALID_URI, base, subpath) == NOT_SUPPORTED, "Splitting invalid uri", __LINE__);
  TestSplitUri(FILE_URI_EMPTY, FILE_URI_EMPTY, EMPTY, "uri without subpath");
  TestSplitUri(FILE_URI1, FILE_URI1_BASE, FILE_URI_SUBPATH, "windows file uri");
  TestSplitUri(FILE_URI2, FILE_URI2_BASE, FILE_URI_SUBPATH, "posix file uri");
  TestSplitUri(FILE_URI3, FILE_URI3_BASE, FILE_URI_SUBPATH, "neutral file uri");
  TestSplitUri(FILE_URI4, FILE_URI4_BASE, FILE_URI_SUBPATH, "complex file uri");
  TestSplitUri(FILE_URI5, FILE_URI5_BASE, FILE_URI_SUBPATH, "file uri with scheme");
  TestSplitUri(HTTP_URI, HTTP_URI_BASE, HTTP_URI_SUBPATH, "http uri");
  Test(SplitUri(FILE_URI_NO, base, subpath) == NOT_SUPPORTED, "Splitting uri with no base uri", __LINE__);
  
  std::cout << "------ test for combiners --------\n";
  Test(CombineUri(INVALID_URI, FILE_URI_SUBPATH, base) == NOT_SUPPORTED, "Combining invalid uri", __LINE__);
  Test(!CombineUri(FILE_URI1, FILE_URI_SUBPATH, base) && base == FILE_URI1, "Combining redundant uri", __LINE__);
}
