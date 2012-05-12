/**
*
* @file     shared_library_gate.h
* @brief    Shared library gate
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SHARED_LIBRARY_GATE_H_DEFINED__
#define __SHARED_LIBRARY_GATE_H_DEFINED__

//common includes
#include <shared_library.h>
//boost includes
#include <boost/static_assert.hpp>

template<class Traits>
class SharedLibraryGate
{
public:
  static SharedLibraryGate& Instance()
  {
    static SharedLibraryGate<Traits> self;
    return self;
  }
  
  template<class T>
  T* GetSymbol(T* wrapper, const char* name) const
  {
    BOOST_STATIC_ASSERT(sizeof(wrapper) == sizeof(void*));
    if (void* const sym = FindSymbol(reinterpret_cast<void*>(wrapper)))
    {
      return reinterpret_cast<T*>(sym);
    }
    void* const sym = Library->GetSymbol(name);
    AddSymbol(reinterpret_cast<void*>(wrapper), sym);
    return reinterpret_cast<T*>(sym);
  }
  
  bool IsAccessible() const
  {
    return Library;
  }

  Error GetLoadError() const
  {
    return LoadError;
  }
private:
  SharedLibraryGate()
  {
    try
    {
      Library = SharedLibrary::Load(Traits::GetName());
      Traits::Startup();
    }
    catch (const Error& e)
    {
      LoadError = e;
    }
  }

  ~SharedLibraryGate()
  {
    if (Library)
    {
      Traits::Shutdown();
    }
  }
  
  SharedLibraryGate(const SharedLibraryGate<Traits>&);
  SharedLibraryGate<Traits>& operator = (const SharedLibraryGate<Traits>&);
  
  void* FindSymbol(void* gate) const
  {
    const GateToSymbol::const_iterator it = Symbols.find(gate);
    return it != Symbols.end()
      ? it->second
      : 0;
  }
  
  void AddSymbol(void* gate, void* symbol) const
  {
    Symbols.insert(GateToSymbol::value_type(gate, symbol));
  }
private:
  SharedLibrary::Ptr Library;
  Error LoadError;
  typedef std::map<void*, void*> GateToSymbol;
  mutable GateToSymbol Symbols;
};

#endif //__SHARED_LIBRARY_GATE_H_DEFINED__
