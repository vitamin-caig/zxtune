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

// common includes
#include <types.h>

namespace Urls
{
  constexpr StringView Email()
  {
    return "zxtune@gmail.com"_sv;
  }

  constexpr StringView Site()
  {
    return "https://zxtune.ru/"_sv;
  }

  inline String DownloadsFeed()
  {
    return Site().to_string() + "downloads.xml";
  }

  inline String Help()
  {
    return Site().to_string() + "manuals/zxtune-qt/";
  }

  inline String Faq()
  {
    return Site().to_string() + "faq/";
  }

  constexpr StringView Repository()
  {
    return "https://bitbucket.org/zxtune/zxtune/"_sv;
  }

  inline String Bugreport()
  {
    return Repository().to_string() + "issues/new";
  }
}  // namespace Urls
