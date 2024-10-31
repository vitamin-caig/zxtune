/**
 *
 * @file
 *
 * @brief App urls set
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <string_type.h>
#include <string_view.h>

namespace Urls
{
  // TODO: use Strings::Concat
  constexpr StringView Email()
  {
    return "zxtune@gmail.com"sv;
  }

  constexpr StringView Site()
  {
    return "https://zxtune.ru/"sv;
  }

  inline String DownloadsFeed()
  {
    return Site() + "downloads.xml"s;
  }

  inline String Help()
  {
    return Site() + "manuals/zxtune-qt/"s;
  }

  inline String Faq()
  {
    return Site() + "faq/"s;
  }

  constexpr StringView Repository()
  {
    return "https://bitbucket.org/zxtune/zxtune/"sv;
  }

  inline String Bugreport()
  {
    return Repository() + "issues/new"s;
  }
}  // namespace Urls
