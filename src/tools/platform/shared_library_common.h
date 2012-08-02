/*
Abstract:
  Common shared library internal interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef TOOLS_SHARED_LIBRARY_COMMON_H_DEFINED
#define TOOLS_SHARED_LIBRARY_COMMON_H_DEFINED

//common includes
#include <error.h>
#include <shared_library.h>
//std includes
#include <vector>

std::string GetSharedLibraryFilename(const std::string& name);
std::vector<std::string> GetSharedLibraryFilenames(const SharedLibrary::Name& name);
Error LoadSharedLibrary(const std::string& fileName, SharedLibrary::Ptr& res);

#endif //TOOLS_SHARED_LIBRARY_COMMON_H_DEFINED
