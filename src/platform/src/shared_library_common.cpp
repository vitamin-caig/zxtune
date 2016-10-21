/**
*
* @file
*
* @brief  Common SharedLibrary logic implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "shared_library_common.h"
//common includes
#include <error_tools.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>

#define FILE_TAG 098808A4

namespace
{
  const Debug::Stream Dbg("Platform");

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("platform");
}

namespace Platform
{
  SharedLibrary::Ptr SharedLibrary::Load(const std::string& name)
  {
    const std::string& fileName = Details::GetSharedLibraryFilename(name);
    SharedLibrary::Ptr res;
    ThrowIfError(Details::LoadSharedLibrary(fileName, res));
    Dbg("Loaded '%1%' (as '%2%')", name, fileName);
    return res;
  }

  SharedLibrary::Ptr SharedLibrary::Load(const SharedLibrary::Name& name)
  {
    const std::vector<std::string> filenames = Details::GetSharedLibraryFilenames(name);
    Error resError = MakeFormattedError(THIS_LINE,
      translate("Failed to load dynamic library '%1%' by any of the alternative names."), FromStdString(name.Base()));
    for (const auto& file : filenames)
    {
      SharedLibrary::Ptr res;
      if (const Error& err = Details::LoadSharedLibrary(file, res))
      {
        resError.AddSuberror(err);
      }
      else
      {
        Dbg("Loaded '%1%' (as '%2%')", name.Base(), file);
        return res;
      }
    }
    throw resError;
    //workaround for MSVS7.1
    return SharedLibrary::Ptr();
  }
}
