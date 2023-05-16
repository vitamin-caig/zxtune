/**
 *
 * @file
 *
 * @brief  Common SharedLibrary logic interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <error.h>
// library includes
#include <platform/shared_library.h>

namespace Platform::Details
{
  String GetSharedLibraryFilename(StringView name);
  std::vector<String> GetSharedLibraryFilenames(const SharedLibrary::Name& name);
  Error LoadSharedLibrary(const String& fileName, SharedLibrary::Ptr& res);
}  // namespace Platform::Details
