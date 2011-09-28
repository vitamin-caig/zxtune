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

  const char NET_URI[] = "//network-path/share/filename?part1/part2";
  const char NET_URI_BASE[] = "//network-path/share/filename";
  const char NET_URI_SUBPATH[] = "part1/part2";

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

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2%\n\tAt: %3%\n\t--------\n") % text % Error::CodeToString(code) % Error::LocationToString(loc)).str();
    std::cerr << txt;
  }
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      e.WalkSuberrors(ErrOuter);
    }
    return e;
  }

  void Test(const Error& res, const String& text, unsigned line)
  {
    std::cout << (res ? "Failed" : "Passed") << " test '" << text << "' at " << line << std::endl;
    ShowIfError(res);
    if (res)
      throw 1;
  }

  void Test(bool res, const String& text, unsigned line)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' at " << line << std::endl;
    if (!res)
      throw 1;
  }

  void TestEq(const String& str, const String& ref, const String& text, unsigned line)
  {
    const bool res = ref == str;
    Test(res, res ? text : text + "('" + str + "'!='" + ref + "')" , line);
  }
  
  void OutProvider(ZXTune::IO::Provider::Ptr info)
  {
    std::cout << 
      "Provider: " << info->Id() << std::endl <<
      "Description: " << info->Description() << std::endl;
  }
  
  void TestSplitUri(const String& uri, const String& baseEq, const String& subEq, const String& type)
  {
    String base, subpath;
    Test(ZXTune::IO::SplitUri(uri, base, subpath), String("Splitting ") + type, __LINE__);
    TestEq(base, baseEq, " testing base", __LINE__);
    TestEq(subpath, subEq, " testing subpath", __LINE__);
    String result;
    Test(ZXTune::IO::CombineUri(base, subpath, result), String("Combining ") + type, __LINE__);
    Test(String::npos != uri.find(result), String(" testing result"), __LINE__);
  }
}

int main()
{
  try
  {
  using namespace ZXTune::IO;
  std::cout << "------ test for enumeration -------\n";

  Provider::Iterator::Ptr providers;
  for (providers = EnumerateProviders(); providers->IsValid(); providers->Next())
  {
    OutProvider(providers->Get());
  }
  String base, subpath;
  std::cout << "------ test for splitters/combiners ------\n";
  Test(SplitUri(INVALID_URI, base, subpath) == ERROR_NOT_SUPPORTED, "Splitting invalid uri", __LINE__);
  TestSplitUri(FILE_URI_EMPTY, FILE_URI_EMPTY, EMPTY, "uri without subpath");
  TestSplitUri(NET_URI, NET_URI_BASE, NET_URI_SUBPATH, "windows network uri");
  TestSplitUri(FILE_URI1, FILE_URI1_BASE, FILE_URI_SUBPATH, "windows file uri");
  TestSplitUri(FILE_URI2, FILE_URI2_BASE, FILE_URI_SUBPATH, "posix file uri");
  TestSplitUri(FILE_URI3, FILE_URI3_BASE, FILE_URI_SUBPATH, "neutral file uri");
  TestSplitUri(FILE_URI4, FILE_URI4_BASE, FILE_URI_SUBPATH, "complex file uri");
  TestSplitUri(FILE_URI5, FILE_URI5_BASE, FILE_URI_SUBPATH, "file uri with scheme");
  Test(SplitUri(FILE_URI_NO, base, subpath) == ERROR_NOT_SUPPORTED, "Splitting uri with no base uri", __LINE__);
  
  std::cout << "------ test for combiners --------\n";
  Test(CombineUri(INVALID_URI, FILE_URI_SUBPATH, base) == ERROR_NOT_SUPPORTED, "Combining invalid uri", __LINE__);
  Test(!CombineUri(FILE_URI1, FILE_URI_SUBPATH, base) && base == FILE_URI1, "Combining redundant uri", __LINE__);
  }
  catch (int code)
  {
    return code;
  }
}
