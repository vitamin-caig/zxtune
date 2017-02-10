/**
*
* @file
*
* @brief  Additional files resolving basic algorithm implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <error_tools.h>
//library includes
#include <core/additional_files_resolve.h>
#include <l10n/api.h>
//boost includes
#include <boost/algorithm/string/join.hpp>

#define FILE_TAG 3BC2770E

namespace Module
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");

  void ResolveAdditionalFiles(const AdditionalFilesSource& source, const Module::AdditionalFiles& files)
  {
    try
    {
      auto filenames = files.Enumerate();
      while (!filenames.empty())
      {
        for (const auto& name : filenames)
        {
          const_cast<AdditionalFiles&>(files).Resolve(name, source.Get(name));
        }
        auto newFilenames = files.Enumerate();
        if (newFilenames == filenames)
        {
          throw MakeFormattedError(THIS_LINE, translate("None of the additional files %1% were resolved."), boost::algorithm::join(filenames, ","));
        }
        filenames.swap(newFilenames);
      }
    }
    catch (const Error& err)
    {
      throw Error(THIS_LINE, translate("Failed to resolve additional files.")).AddSuberror(err);
    }
  }
}
