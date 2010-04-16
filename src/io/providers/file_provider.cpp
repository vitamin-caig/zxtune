/*
Abstract:
  Providers enumerator

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"

#include <tools.h>
#include <error_tools.h>
#include <io/error_codes.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>

#include <fstream>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/shared_array.hpp>

#include <io/text/io.h>

#define FILE_TAG 0D4CB3DA

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::IO;

  const String PROVIDER_VERSION(FromStdString("$Rev$"));

  // provider information structure
  const ProviderInformation PROVIDER_INFO =
  {
    Text::IO_FILE_PROVIDER_NAME,
    Text::IO_FILE_PROVIDER_DESCRIPTION,
    PROVIDER_VERSION,
  };

  // uri-related constants
  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_FILE[] = {'f', 'i', 'l', 'e', 0};
  const Char SUBPATH_DELIMITER = '\?';

  class FileDataContainer : public DataContainer
  {
    // basic interface for internal data storing
    class Holder
    {
    public:
      typedef boost::shared_ptr<Holder> Ptr;
      virtual ~Holder() {}
      
      virtual std::size_t Size() const = 0;
      virtual const uint8_t* Data() const = 0;
    };

    // memory-mapping holder implementation
    class MMapHolder : public Holder
    {
    public:
      explicit MMapHolder(const String& path)
      try
        : File(ConvertToFilename(path).c_str(), boost::interprocess::read_only), Region(File, boost::interprocess::read_only)
      {
      }
      catch (const boost::interprocess::interprocess_exception& e)
      {
        throw Error(THIS_LINE, ERROR_IO_ERROR, FromStdString(e.what()));
      }

      virtual std::size_t Size() const
      {
        return Region.get_size();
      }
      
      virtual const uint8_t* Data() const
      {
        return static_cast<const uint8_t*>(Region.get_address());
      }
    private:
      boost::interprocess::file_mapping File;
      boost::interprocess::mapped_region Region;
    };

    // simple buffer implementation
    class DumpHolder : public Holder
    {
    public:
      DumpHolder(const boost::shared_array<uint8_t>& dump, std::size_t size)
        : Dump(dump), Length(size)
      {
      }
      
      virtual std::size_t Size() const
      {
        return Length;
      }
      
      virtual const uint8_t* Data() const
      {
        return Dump.get();
      }
    private:
      const boost::shared_array<uint8_t> Dump;
      const std::size_t Length;
    };
  public:
    FileDataContainer(const String& path, const Parameters::Map& params)
      : CoreHolder()
      , Offset(0), Length(0)
    {
      //TODO: possible use boost.filesystem to determine file size
      std::ifstream file(ConvertToFilename(path).c_str(), std::ios::binary);
      if (!file)
      {
        throw Error(THIS_LINE, ERROR_NO_ACCESS, Text::IO_ERROR_NO_ACCESS);
      }
      file.seekg(0, std::ios::end);
      const std::streampos fileSize = file.tellg();
      if (!fileSize || !file)
      {
        throw Error(THIS_LINE, ERROR_IO_ERROR, Text::IO_ERROR_IO_ERROR);
      }
      std::streampos threshold = static_cast<std::streampos>(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT);
      if (const Parameters::IntType* val =
        Parameters::FindByName<Parameters::IntType>(params, Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD))
      {
        threshold = static_cast<std::streampos>(*val);
      }
      if (fileSize >= threshold)
      {
        file.close();
        //use mmap
        CoreHolder.reset(new MMapHolder(path));
      }
      else
      {
        boost::shared_array<uint8_t> buffer(new uint8_t[fileSize]);
        file.seekg(0);
        file.read(safe_ptr_cast<char*>(buffer.get()), std::streamsize(fileSize));
        file.close();
        //use dump
        CoreHolder.reset(new DumpHolder(buffer, fileSize));
      }
      Length = fileSize;
    }
    
    virtual std::size_t Size() const
    {
      return Length;
    }
    
    virtual const void* Data() const
    {
      return CoreHolder->Data() + Offset;
    }
    
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      size = std::min(size, Length - offset);
      return Ptr(new FileDataContainer(CoreHolder, offset + Offset, size));
    }
  private:
    FileDataContainer(Holder::Ptr holder, std::size_t offset, std::size_t size)
      : CoreHolder(holder), Offset(offset), Length(size)
    {
    }
    
  private:
    Holder::Ptr CoreHolder;
    const std::size_t Offset;
    std::size_t Length;
  };

  inline bool IsOrdered(String::size_type lh, String::size_type rh)
  {
    return String::npos == rh ? true : lh < rh;
  }

  ///////////////////////////////////////
  bool FileChecker(const String& uri)
  {
    // TODO: extract and use common scheme-working code
    const String::size_type schemePos = uri.find(SCHEME_SIGN);
    const String::size_type basePos = String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1;
    const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER);
    
    return (String::npos == schemePos || uri.substr(0, schemePos) == SCHEME_FILE) && IsOrdered(basePos, subPos);
  }
  
  Error FileSplitter(const String& uri, String& baseUri, String& subpath)
  {
    const String::size_type schemePos = uri.find(SCHEME_SIGN);
    const String::size_type basePos = String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1;
    const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER);
    if ((String::npos != schemePos && uri.substr(0, schemePos) != SCHEME_FILE) || !IsOrdered(basePos, subPos))
    {
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }
    if (String::npos != subPos)
    {
      baseUri = uri.substr(basePos, subPos - basePos);
      subpath = uri.substr(subPos + 1);
    }
    else
    {
      baseUri = uri.substr(basePos);
      subpath = String();
    }
    return Error();
  }
  
  Error FileCombiner(const String& baseUri, const String& subpath, String& uri)
  {
    String base, sub;
    if (const Error& e = FileSplitter(baseUri, base, sub))
    {
      return e;
    }
    uri = base;
    if (!subpath.empty())
    {
      uri += SUBPATH_DELIMITER;
      uri += subpath;
    }
    return Error();
  }
  
  //no callback
  Error FileOpener(const String& uri, const Parameters::Map& params, const ProgressCallback& /*cb*/,
    DataContainer::Ptr& result, String& subpath)
  {
    String openUri, openSub;
    if (const Error& e = FileSplitter(uri, openUri, openSub))
    {
      return e;
    }
    try
    {
      result = DataContainer::Ptr(new FileDataContainer(openUri, params));
      subpath = openSub;
      return Error();
    }
    catch (const Error& e)
    {
      return MakeFormattedError(THIS_LINE, ERROR_NOT_OPENED, Text::IO_ERROR_NOT_OPENED, openUri).AddSuberror(e);
    }
  }
}

namespace ZXTune
{
  namespace IO
  {
    void RegisterFileProvider(ProvidersEnumerator& enumerator)
    {
      enumerator.RegisterProvider(
        PROVIDER_INFO,
        FileChecker, FileOpener, FileSplitter, FileCombiner);
    }
  }
}
