/*
Abstract:
  Common shared library internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef PLATFORM_SHARED_LIBRARY_COMMON_H_DEFINED
#define PLATFORM_SHARED_LIBRARY_COMMON_H_DEFINED

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

#endif //PLATFORM_SHARED_LIBRARY_COMMON_H_DEFINED
