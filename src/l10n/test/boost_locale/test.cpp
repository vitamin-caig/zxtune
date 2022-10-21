/**
 *
 * @file
 *
 * @brief  L10n test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <cstring>
#include <fstream>
#include <iostream>
#include <l10n/api.h>
#include <pointers.h>
#include <sstream>
#include <strings/format.h>

namespace
{
  const std::string Domain("test");

  Binary::Dump OpenFile(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Binary::Dump tmp(size);
    stream.read(safe_ptr_cast<char*>(tmp.data()), tmp.size());
    return tmp;
  }

  std::string Test(const std::string& domain)
  {
    const L10n::TranslateFunctor translate(domain);
    std::ostringstream str;
    str << translate("Just a message") << "\r\n";
    str << translate("context", "Just a message with context") << "\r\n";
    for (uint_t idx = 0; idx != 5; ++idx)
    {
      str << Strings::Format(translate("Single form for {}", "Plural form for {}", idx), idx) << "\r\n";
      str << Strings::Format(
          translate("another context", "Single form for {} with context", "Plural form for {} with context", idx), idx)
          << "\r\n";
    }
    return str.str();
  }

  void Test(const Binary::Dump& ref)
  {
    const std::string val = Test(Domain);
    const std::string refStr(ref.begin(), ref.end());
    std::cout << val.size() << " bytes:\n" << val;
    if (val == refStr)
    {
      return;
    }
    std::cout << "Failed!\n" << refStr.size() << " bytes:\n";
    if (val.size() <= refStr.size())
    {
      const std::pair<std::string::const_iterator, std::string::const_iterator> mis =
          std::mismatch(val.begin(), val.end(), refStr.begin());
      if (mis.first == val.end())
      {
        std::cout << "missed tail " << refStr.end() - mis.second << " bytes:\n"
                  << std::string(mis.second, refStr.end()) << std::endl;
      }
      else
      {
        std::cout << "mismatch at " << std::distance(val.begin(), mis.first) << ": res=" << unsigned(*mis.first)
                  << " ref=" << unsigned(*mis.second) << std::endl;
      }
    }
    else
    {
      const std::pair<std::string::const_iterator, std::string::const_iterator> mis =
          std::mismatch(refStr.begin(), refStr.end(), val.begin());
      if (mis.first == refStr.end())
      {
        std::cout << "redundand tail " << val.end() - mis.second << " bytes:\n"
                  << std::string(mis.second, val.end()) << std::endl;
      }
      else
      {
        std::cout << "mismatch at " << std::distance(refStr.begin(), mis.first) << ": res=" << unsigned(*mis.first)
                  << " ref=" << unsigned(*mis.second) << std::endl;
      }
    }
  }
}  // namespace

int main()
{
  try
  {
    L10n::Library& library = L10n::Library::Instance();
    const L10n::Translation eng = {Domain, "en", "mo", OpenFile("en/test.mo")};
    library.AddTranslation(eng);
    const L10n::Translation rus = {Domain, "ru", "mo", OpenFile("ru/test.mo")};
    library.AddTranslation(rus);

    std::cout << "Test Default translation" << std::endl;
    Test(OpenFile("default.res"));
    std::cout << "Test English translation" << std::endl;
    library.SelectTranslation("en");
    Test(OpenFile("english.res"));
    std::cout << "Test Russian translation" << std::endl;
    library.SelectTranslation("ru");
    Test(OpenFile("russian.res"));
    std::cout << "Test nonexisting translation" << std::endl;
    library.SelectTranslation("zz");
    Test(OpenFile("default.res"));
    return 0;
  }
  catch (...)
  {
    return 1;
  }
}
