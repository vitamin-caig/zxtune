/**
 *
 * @file
 *
 * @brief  Common SharedLibrary logic implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "platform/src/shared_library_common.h"

#include "debug/log.h"
#include "l10n/api.h"

#include "error_tools.h"
#include "string_view.h"

namespace
{
  const Debug::Stream Dbg("Platform");

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("platform");
}  // namespace

namespace Platform
{
  SharedLibrary::Ptr SharedLibrary::Load(StringView name)
  {
    const auto fileName = Details::GetSharedLibraryFilename(name);
    SharedLibrary::Ptr res;
    ThrowIfError(Details::LoadSharedLibrary(fileName, res));
    Dbg("Loaded '{}' (as '{}')", name, fileName);
    return res;
  }

  SharedLibrary::Ptr SharedLibrary::Load(const SharedLibrary::Name& name)
  {
    const auto filenames = Details::GetSharedLibraryFilenames(name);
    Error resError = MakeFormattedError(
        THIS_LINE, translate("Failed to load dynamic library '{}' by any of the alternative names."), name.Base());
    for (const auto& file : filenames)
    {
      SharedLibrary::Ptr res;
      if (const Error& err = Details::LoadSharedLibrary(file, res))
      {
        resError.AddSuberror(err);
      }
      else
      {
        Dbg("Loaded '{}' (as '{}')", name.Base(), file);
        return res;
      }
    }
    throw resError;
    // workaround for MSVS7.1
    return {};
  }
}  // namespace Platform
