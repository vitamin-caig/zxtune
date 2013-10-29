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

//common includes
#include <error.h>
//library includes
#include <platform/shared_library.h>

namespace Platform
{
  namespace Details
  {
    std::string GetSharedLibraryFilename(const std::string& name);
    std::vector<std::string> GetSharedLibraryFilenames(const SharedLibrary::Name& name);
    Error LoadSharedLibrary(const std::string& fileName, SharedLibrary::Ptr& res);
  }
}
