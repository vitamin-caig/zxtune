/*
Abstract:
  Providers enumerator

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"

#include <io/error_codes.h>
#include <io/providers_parameters.h>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <fstream>

#include <text/io.h>

#define FILE_TAG 0D4CB3DA

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::IO;

  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_FILE[] = {'f', 'i', 'l', 'e', 0};
  const Char SUBPATH_DELIMITER = '\?';

  class FileDataContainer : public DataContainer
  {
    class Holder
    {
    public:
      typedef boost::shared_ptr<Holder> Ptr;
      virtual ~Holder() {}
      
      virtual std::size_t Size() const = 0;
      virtual const uint8_t* Data() const = 0;
    };
    
    class MMapHolder : public Holder
    {
    public:
      explicit MMapHolder(const String& path)
      try
        : File(path.c_str(), boost::interprocess::read_only), Region(File, boost::interprocess::read_only)
      {
      }
      catch (const boost::interprocess::interprocess_exception& e)
      {
        throw Error(THIS_LINE, IO_ERROR, e.what());
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
    
    class SmartFileHolder : public Holder
    {
    public:
      SmartFileHolder(const String& path, const ParametersMap& params)
        : Delegate(0)
      {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file)
        {
          throw Error(THIS_LINE, NO_ACCESS, TEXT_IO_ERROR_NO_ACCESS);
        }
        file.seekg(0, std::ios::end);
        const std::streampos fileSize = file.tellg();
        if (!fileSize || !file)
        {
          throw Error(THIS_LINE, IO_ERROR, TEXT_IO_ERROR_IO_ERROR);
        }
        int64_t threshold = Parameters::IO::Providers::File::MMAP_THRESHOLD_DEFAULT;
        if (const int64_t* val = FindParameter<int64_t>(params, Parameters::IO::Providers::File::MMAP_THRESHOLD))
        {
          threshold = *val;
        }
        if (threshold >= fileSize)
        {
          file.close();
          //use mmap
          Delegate = new MMapHolder(path);
        }
        else
        {
          Buffer.resize(fileSize);
          file.seekg(0);
          file.read(safe_ptr_cast<char*>(&Buffer[0]), std::streamsize(Buffer.size()));
        }
      }
      
      virtual std::size_t Size() const
      {
        return Delegate ? Delegate->Size() : Buffer.size();
      }
      
      virtual const uint8_t* Data() const
      {
        return Delegate ? Delegate->Data() : &Buffer[0];
      }
    private:
      Holder* Delegate;
      Dump Buffer;
    };
  public:
    FileDataContainer(const String& path, const ParametersMap& params)
      : CoreHolder(new SmartFileHolder(path, params))
      , Offset(0), Length(CoreHolder->Size())
    {
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
    const std::size_t Length;
  };


  inline bool IsOrdered(String::size_type lh, String::size_type rh)
  {
    return String::npos == rh ? true : lh < rh;
  }

  ///////////////////////////////////////
  bool FileChecker(const String& uri)
  {
    const String::size_type schemePos(uri.find(SCHEME_SIGN));
    const String::size_type basePos(String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1);
    const String::size_type subPos(uri.find_first_of(SUBPATH_DELIMITER));
    
    return (String::npos == schemePos || uri.substr(0, schemePos) == SCHEME_FILE) && IsOrdered(basePos, subPos);
  }
  
  Error FileSplitter(const String& uri, String& baseUri, String& subpath)
  {
    const String::size_type schemePos(uri.find(SCHEME_SIGN));
    const String::size_type basePos(String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1);
    const String::size_type subPos(uri.find_first_of(SUBPATH_DELIMITER));
    if ((String::npos != schemePos && uri.substr(0, schemePos) != SCHEME_FILE) || !IsOrdered(basePos, subPos))
    {
      return Error(THIS_LINE, NOT_SUPPORTED, TEXT_IO_ERROR_NOT_SUPPORTED_URI);
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
    if (!sub.empty())
    {
      return Error(THIS_LINE, NOT_SUPPORTED, TEXT_IO_ERROR_NOT_SUPPORTED_URI);
    }
    uri = baseUri;
    if (!subpath.empty())
    {
      uri += SUBPATH_DELIMITER;
      uri += subpath;
    }
    return Error();
  }
  
  //no callback
  Error FileOpener(const String& uri, const ParametersMap& params, const ProgressCallback& /*cb*/, DataContainer::Ptr& result, String& subpath)
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
      return MakeFormattedError(THIS_LINE, NOT_OPENED, TEXT_IO_ERROR_NOT_OPENED, openUri).AddSuberror(e);
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
        ProviderInfo(TEXT_IO_FILE_PROVIDER_NAME, TEXT_IO_FILE_PROVIDER_DESCRIPTION),
        FileChecker, FileOpener, FileSplitter, FileCombiner);
    }
  }
}
