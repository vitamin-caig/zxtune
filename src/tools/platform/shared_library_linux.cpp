/*
Abstract:
  SharedLibrary implementation for Linux

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <contract.h>
#include <error_tools.h>
#include <shared_library.h>
//boost includes
#include <boost/make_shared.hpp>
//platform includes
#include <dlfcn.h>
//text includes
#include <tools/text/tools.h>

#define FILE_TAG 4C0042B0

namespace
{
  const unsigned THIS_MODULE = Error::ModuleCode<'L', 'S', 'O'>::Value;

  void CloseLibrary(void* handle)
  {
    if (handle)
    {
      ::dlclose(handle);
    }
  }
  
  typedef boost::shared_ptr<void> LibHandle;
  
  LibHandle OpenLibrary(const std::string& fileName)
  {
    return LibHandle(::dlopen(fileName.c_str(), RTLD_LAZY), std::ptr_fun(&CloseLibrary));
  }
  
  Error CreateLoadError(const Error::LocationRef& loc, const std::string& fileName)
  {
    return MakeFormattedError(loc, THIS_MODULE,
      Text::FAILED_LOAD_DYNAMIC_LIBRARY, FromStdString(fileName), FromStdString(::dlerror()));
  }

  class LinuxSharedLibrary : public SharedLibrary
  {
  public:
    explicit LinuxSharedLibrary(LibHandle handle)
      : Handle(handle)
    {
      Require(Handle);
    }

    virtual void* GetSymbol(const std::string& name) const
    {
      if (void* res = ::dlsym(Handle.get(), name.c_str()))
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, THIS_MODULE, Text::FAILED_FIND_DYNAMIC_LIBRARY_SYMBOL, FromStdString(name));
    }
  private:
    const LibHandle Handle;
  };
  
  const std::string SUFFIX(".so");
  
  std::string BuildLibraryFilename(const std::string& name)
  {
    return "lib" + name + SUFFIX;
  }
  
  std::string GetLibraryFilename(const std::string& name)
  {
    return name.find(SUFFIX) == name.npos
      ? BuildLibraryFilename(name)
      : name;
  }
}

SharedLibrary::Ptr SharedLibrary::Load(const std::string& name)
{
  const std::string& fileName = GetLibraryFilename(name);
  if (const LibHandle handle = OpenLibrary(fileName))
  {
    return boost::make_shared<LinuxSharedLibrary>(handle);
  }
  throw CreateLoadError(THIS_LINE, fileName);
}

SharedLibrary::Ptr SharedLibrary::Load(const SharedLibrary::Name& name)
{
  const std::string& baseFileName = GetLibraryFilename(name.Base());
  if (const LibHandle handle = OpenLibrary(baseFileName))
  {
    return boost::make_shared<LinuxSharedLibrary>(handle);
  }
  const Error result = CreateLoadError(THIS_LINE, name.Base());
  const std::vector<std::string>& alternatives = name.PosixAlternatives();
  for (std::vector<std::string>::const_iterator it = alternatives.begin(), lim = alternatives.end(); it != lim; ++it)
  {
    const std::string altName = *it;
    const std::string altFilename = GetLibraryFilename(altName);
    if (const LibHandle handle = OpenLibrary(altFilename))
    {
      return boost::make_shared<LinuxSharedLibrary>(handle);
    }
  }
  throw result;
}
