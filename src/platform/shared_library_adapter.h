/**
*
* @file     shared_library_adapter.h
* @brief    Shared library adapter
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PLATFORM_SHARED_LIBRARY_ADAPTER_H_DEFINED
#define PLATFORM_SHARED_LIBRARY_ADAPTER_H_DEFINED

//library includes
#include <platform/shared_library.h>
//std includes
#include <map>

namespace Platform
{
  class SharedLibraryAdapter
  {
  public:
    explicit SharedLibraryAdapter(SharedLibrary::Ptr lib)
      : Library(lib)
    {
    }

    template<class F>
    F GetSymbol(const char* name) const
    {
      const NameToSymbol::const_iterator it = Symbols.find(name);
      if (it != Symbols.end())
      {
        return reinterpret_cast<F>(it->second);
      }
      void* const symbol = Library->GetSymbol(name);
      Symbols.insert(NameToSymbol::value_type(name, symbol));
      return reinterpret_cast<F>(symbol);
    }
  private:
    const SharedLibrary::Ptr Library;
    typedef std::map<const char*, void*> NameToSymbol;
    mutable NameToSymbol Symbols;
  };
}

#endif //PLATFORM_SHARED_LIBRARY_ADAPTER_H_DEFINED
