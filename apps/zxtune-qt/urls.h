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
    return String{Site()} + "downloads.xml";
  }

  inline String Help()
  {
    return String{Site()} + "manuals/zxtune-qt/";
  }

  inline String Faq()
  {
    return String{Site()} + "faq/";
  }

  constexpr StringView Repository()
  {
    return "https://bitbucket.org/zxtune/zxtune/"_sv;
  }

  inline String Bugreport()
  {
    return String{Repository()} + "issues/new";
  }
}  // namespace Urls
