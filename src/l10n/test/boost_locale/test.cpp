/**
 *
 * @file
 *
 * @brief  L10n test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "test/utils.h"

#include "binary/container_factories.h"
#include "l10n/api.h"
#include "strings/format.h"

#include "pointers.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
  const std::string Domain("test");

  Binary::Container::Ptr ReadFile(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    auto tmp = std::make_unique<Binary::Dump>(size);
    stream.read(safe_ptr_cast<char*>(tmp->data()), tmp->size());
    return Binary::CreateContainer(std::move(tmp));
  }

  L10n::Translation MakeTranslation(const std::string& language, const std::string& type = "mo")
  {
    L10n::Translation res;
    res.Domain = Domain;
    res.Language = language;
    res.Type = type;
    res.Data = ReadFile(language + "/test." + type);
    return res;
  }

  std::vector<std::string> ReadLines(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::in);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    std::vector<std::string> result;
    std::vector<char> buf(512);
    while (stream.getline(buf.data(), buf.size()))
    {
      result.emplace_back(buf.data());
    }
    return result;
  }

  std::vector<std::string> Test(const std::string& domain)
  {
    const L10n::TranslateFunctor translate(domain);
    std::vector<std::string> result;
    result.emplace_back(translate("Just a message"));
    result.emplace_back(translate("context", "Just a message with context"));
    for (uint_t idx = 0; idx != 5; ++idx)
    {
      result.emplace_back(Strings::FormatRuntime(translate("Single form for {}", "Plural form for {}", idx), idx));
      result.emplace_back(Strings::FormatRuntime(
          translate("another context", "Single form for {} with context", "Plural form for {} with context", idx),
          idx));
    }
    return result;
  }

  void Test(const std::vector<std::string>& ref)
  {
    const auto& val = Test(Domain);
    if (val == ref)
    {
      return;
    }
    std::cout << "Failed!\n Ref: " << ref << "\n Tst: " << val << std::endl;
    // throw 1;
  }
}  // namespace

int main()
{
  try
  {
    L10n::Library& library = L10n::Library::Instance();
    library.AddTranslation(MakeTranslation("en"));
    library.AddTranslation(MakeTranslation("ru"));

    std::cout << "Test Default translation" << std::endl;
    Test(ReadLines("default.res"));
    std::cout << "Test English translation" << std::endl;
    library.SelectTranslation("en");
    Test(ReadLines("english.res"));
    std::cout << "Test Russian translation" << std::endl;
    library.SelectTranslation("ru");
    Test(ReadLines("russian.res"));
    std::cout << "Test nonexisting translation" << std::endl;
    library.SelectTranslation("zz");
    Test(ReadLines("default.res"));
    return 0;
  }
  catch (...)
  {
    return 1;
  }
}
