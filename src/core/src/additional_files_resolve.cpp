/**
 *
 * @file
 *
 * @brief  Additional files resolving basic algorithm implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/src/l10n.h"
// common includes
#include <error_tools.h>
// library includes
#include <core/additional_files_resolve.h>
// boost includes
#include <boost/algorithm/string/join.hpp>

namespace Module
{
  void ResolveAdditionalFiles(const AdditionalFilesSource& source, const Module::AdditionalFiles& files)
  {
    try
    {
      auto filenames = files.Enumerate();
      while (!filenames.empty())
      {
        // Try to resolve one-by-one
        const auto name = filenames.front();
        const_cast<AdditionalFiles&>(files).Resolve(name, source.Get(name));
        auto newFilenames = files.Enumerate();
        if (newFilenames == filenames)
        {
          throw MakeFormattedError(THIS_LINE, translate("None of the additional files {} were resolved."),
                                   boost::algorithm::join(filenames, ","));
        }
        filenames.swap(newFilenames);
      }
    }
    catch (const Error& err)
    {
      throw Error(THIS_LINE, translate("Failed to resolve additional files.")).AddSuberror(err);
    }
  }
}  // namespace Module

