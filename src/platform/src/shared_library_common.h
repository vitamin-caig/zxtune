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

#include "platform/shared_library.h"

#include "error.h"
#include "string_type.h"
#include "string_view.h"

#include <vector>

namespace Platform::Details
{
  String GetSharedLibraryFilename(StringView name);
  std::vector<String> GetSharedLibraryFilenames(const SharedLibrary::Name& name);
  Error LoadSharedLibrary(const String& fileName, SharedLibrary::Ptr& res);
}  // namespace Platform::Details
