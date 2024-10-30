/**
 *
 * @file
 *
 * @brief  Providers test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <algorithm>
#include <io/providers/providers_factories.h>
#include <iomanip>
#include <iostream>

namespace
{
  const std::string CHANGED_SUBPATH("Complex/Subpath");

  struct Case
  {
    std::string Name;
    std::string Uri;
    std::string Scheme;
    std::string Path;
    std::string Filename;
    std::string Extension;
    std::string Subpath;
    std::string WithChangedSubpath;
  };

  // clang-format off
  const Case FILE_PROVIDER_CASES[] =
  {
    {
      "Empty string",
      "",
      "",
      "",
      "",
      "",
      "",
      ""
    },
    {
      "Invalid uri",
      "uri://invalid",
      "",
      "",
      "",
      "",
      "",
      ""
    },
    {
      "No path uri",
      "?some_subpath",
      "",
      "",
      "",
      "",
      "",
      ""
    },
    {
      "Single file no subpath",
      "some_file.ext",
      "file",
      "some_file.ext",
      "some_file.ext",
      "ext",
      "",
      "some_file.ext?Complex/Subpath"
    },
    {
      "Single file no extension",
      "some_file",
      "file",
      "some_file",
      "some_file",
      "",
      "",
      "some_file?Complex/Subpath"
    },
    {
      "Single file with subpath",
      "some_file.ext?some_subpath",
      "file",
      "some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "some_file.ext?Complex/Subpath"
    },
    {
      "Single file with subpath no extension",
      "some_file?some_subpath",
      "file",
      "some_file",
      "some_file",
      "",
      "some_subpath",
      "some_file?Complex/Subpath"
    },
    {
      "File in folder with subpath",
      "folder/some_file.ext?some_subpath",
      "file",
      "folder/some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "folder/some_file.ext?Complex/Subpath"
    },
    {
      "Samba file",
      "//network-path/share/some_file.ext?some_subpath",
      "file",
      "//network-path/share/some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "//network-path/share/some_file.ext?Complex/Subpath"
    },
#ifdef _WIN32
    {
      "Windows file",
      "C:\\folder\\some_file.ext?some_subpath",
      "file",
      "C:\\folder\\some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "C:\\folder\\some_file.ext?Complex/Subpath"
    },
#endif
    {
      "Posix file",
      "/folder/some_file.ext?some_subpath",
      "file",
      "/folder/some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "/folder/some_file.ext?Complex/Subpath"
    },
    {
      "Schemed file in folder with subpath",
      "file://folder/some_file.ext?some_subpath",
      "file",
      "folder/some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "folder/some_file.ext?Complex/Subpath"
    },
#ifdef _WIN32
    {
      "Schemed windows file",
      "file://C:\\folder\\some_file.ext?some_subpath",
      "file",
      "C:\\folder\\some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "C:\\folder\\some_file.ext?Complex/Subpath"
    },
#endif
    {
      "Schemed posix file",
      "file:///folder/some_file.ext?some_subpath",
      "file",
      "/folder/some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "/folder/some_file.ext?Complex/Subpath"
    },
    {
      "Relative file",
      "../../some_file.ext?some_subpath",
      "file",
      "../../some_file.ext",
      "some_file.ext",
      "ext",
      "some_subpath",
      "../../some_file.ext?Complex/Subpath"
    }
  };

  const Case NETWORK_PROVIDER_CASES[] =
  {
    {
      "Empty string",
      "",
      "",
      "",
      "",
      "",
      "",
      ""
    },
    {
      "Invalid scheme",
      "uri://invalid",
      "",
      "",
      "",
      "",
      "",
      ""
    },
    {
      "No path uri",
      "http://#subpath",
      "",
      "",
      "",
      "",
      "",
      ""
    },
    {
      "Simple uri no subpath",
      "http://example.com",
      "http",
      "http://example.com",
      "",
      "",
      "",
      "http://example.com#Complex/Subpath"
    },
    {
      "Uri with parameters no subpath",
      "http://example.com?param=val",
      "http",
      "http://example.com?param=val",
      "",
      "",
      "",
      "http://example.com?param=val#Complex/Subpath",
    },
    {
      "Uri with subpath",
      "http://example.com#some_subpath",
      "http",
      "http://example.com",
      "",
      "",
      "some_subpath",
      "http://example.com#Complex/Subpath"
    },
    {
      "Uri with parameters and subpath",
      "http://example.com?param=val#some_subpath",
      "http",
      "http://example.com?param=val",
      "",
      "",
      "some_subpath",
      "http://example.com?param=val#Complex/Subpath",
    }
  };
  // clang-format on

  template<class T>
  void Test(const std::string& msg, T result, T reference)
  {
    if (result == reference)
    {
      std::cout << "Passed test for " << msg << std::endl;
    }
    else
    {
      std::cout << "Failed test for " << msg << "\ngot: '" << result << "'\nexp: '" << reference << '\'' << std::endl;
      throw 1;
    }
  }

  void TestProvider(const IO::DataProvider& provider, const Case& cs)
  {
    if (const IO::Identifier::Ptr id = provider.Resolve(cs.Uri))
    {
      const auto pid = String{provider.Id()} + " ";
      Test(pid + cs.Name + " (scheme)", id->Scheme(), cs.Scheme);
      Test(pid + cs.Name + " (path)", id->Path(), cs.Path);
      Test(pid + cs.Name + " (filename)", id->Filename(), cs.Filename);
      Test(pid + cs.Name + " (extension)", id->Extension(), cs.Extension);
      Test(pid + cs.Name + " (subpath)", id->Subpath(), cs.Subpath);
      Test(pid + cs.Name + " (modify)", id->WithSubpath(CHANGED_SUBPATH)->Full(), cs.WithChangedSubpath);
    }
    else
    {
      Test(cs.Name, std::string(), cs.WithChangedSubpath);
    }
  }

  void TestFileProvider()
  {
    std::cout << "Test for file provider" << std::endl;
    const IO::DataProvider::Ptr prov = IO::CreateFileDataProvider();
    for (const auto& testCase : FILE_PROVIDER_CASES)
    {
      TestProvider(*prov, testCase);
    }
  }

  void TestNetworkProvider()
  {
    std::cout << "Test for network provider" << std::endl;
    const IO::DataProvider::Ptr prov = IO::CreateNetworkDataProvider(IO::Curl::Api::Ptr());
    for (const auto& testCase : NETWORK_PROVIDER_CASES)
    {
      TestProvider(*prov, testCase);
    }
  }
}  // namespace

int main()
{
  try
  {
    TestFileProvider();
    TestNetworkProvider();
  }
  catch (int code)
  {
    return code;
  }
}
