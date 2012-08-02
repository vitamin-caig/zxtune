/*
Abstract:
  Providers enumerator

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
//common includes
#include <tools.h>
#include <error_tools.h>
//library includes
#include <io/error_codes.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
//std includes
#include <fstream>
//boost includes
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/shared_array.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 0D4CB3DA

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::IO;

  class FileProviderParameters
  {
  public:
    explicit FileProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    std::streampos GetMMapThreshold() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, intVal);
      return static_cast<std::streampos>(intVal);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  // uri-related constants
  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_FILE[] = {'f', 'i', 'l', 'e', 0};
  const Char SUBPATH_DELIMITER = '\?';

  class FileDataContainer : public Binary::Container
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
    FileDataContainer(const String& path, const Parameters::Accessor& params)
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
      const FileProviderParameters providerParams(params);
      const std::streampos threshold = providerParams.GetMMapThreshold();

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
  class FileDataProvider : public DataProvider
  {
  public:
    virtual String Id() const
    {
      return Text::IO_FILE_PROVIDER_ID;
    }

    virtual String Description() const
    {
      return Text::IO_FILE_PROVIDER_DESCRIPTION;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual bool Check(const String& uri) const
    {
      // TODO: extract and use common scheme-working code
      const String::size_type schemePos = uri.find(SCHEME_SIGN);
      const String::size_type basePos = String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1;
      const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER);
    
      return (String::npos == schemePos || uri.substr(0, schemePos) == SCHEME_FILE) && IsOrdered(basePos, subPos);
    }
  
    virtual Error Split(const String& uri, String& path, String& subpath) const
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
        path = uri.substr(basePos, subPos - basePos);
        subpath = uri.substr(subPos + 1);
      }
      else
      {
        path = uri.substr(basePos);
        subpath = String();
      }
      return Error();
    }
  
    virtual Error Combine(const String& path, const String& subpath, String& uri) const
    {
      String base, sub;
      if (const Error& e = Split(path, base, sub))
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
    virtual Error Open(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& /*cb*/,
      Binary::Container::Ptr& result) const
    {
      try
      {
        result = Binary::Container::Ptr(new FileDataContainer(path, params));
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, ERROR_NOT_OPENED, Text::IO_ERROR_NOT_OPENED, path).AddSuberror(e);
      }
    }
  };
}

namespace ZXTune
{
  namespace IO
  {
    void RegisterFileProvider(ProvidersEnumerator& enumerator)
    {
      const DataProvider::Ptr provider(new FileDataProvider());
      enumerator.RegisterProvider(provider);
    }
  }
}
