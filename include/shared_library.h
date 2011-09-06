/**
*
* @file     shared_library.h
* @brief    Shared library interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SHARED_LIBRARY_H_DEFINED__
#define __SHARED_LIBRARY_H_DEFINED__

class SharedLibrary
{
public:
  typedef boost::shared_ptr<const SharedLibrary> Ptr;
  virtual ~SharedLibrary() {}

  virtual void* GetSymbol(const std::string& name) const = 0;

  //! @param name Library name without platform-dependent prefixes and extension
  // E.g. Load("SDL") will call LoadImpl("libSDL.so") for Linux and LoadImpl("SDL.dll") for Windows
  static Ptr Load(const std::string& name);
};

#endif //__SHARED_LIBRARY_H_DEFINED__
