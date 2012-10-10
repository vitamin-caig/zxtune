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
#include "file_provider.h"
//common includes
#include <contract.h>
#include <debug_log.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <binary/container_factories.h>
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
#include <l10n/api.h>
//boost includes
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 0D4CB3DA

#undef min

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::IO;

  const Debug::Stream Dbg("IO::Provider::File");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("io");

  class FileProviderParameters : public FileCreatingParameters
  {
  public:
    explicit FileProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    std::size_t MemoryMappingThreshold() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, intVal);
      return static_cast<std::size_t>(intVal);
    }

    virtual bool Overwrite() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, intVal);
      return intVal != 0;
    }

    virtual bool CreateDirectories() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, intVal);
      return intVal != 0;
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  // uri-related constants
  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_FILE[] = {'f', 'i', 'l', 'e', 0};
  const Char SUBPATH_DELIMITER = '\?';

  class FileIdentifier : public Identifier
  {
  public:
    FileIdentifier(const String& path, const String& subpath)
      : PathValue(path)
      , SubpathValue(subpath)
      , FullValue(Serialize())
    {
      Require(!PathValue.empty());
    }

    virtual String Full() const
    {
      return FullValue;
    }

    virtual String Scheme() const
    {
      return SCHEME_FILE;
    }

    virtual String Path() const
    {
      return PathValue;
    }

    virtual String Filename() const
    {
      String rest;
      return ZXTune::IO::ExtractLastPathComponent(PathValue, rest);
    }

    virtual String Extension() const
    {
      const String& filename = Filename();
      const String::size_type dotPos = filename.find_last_of('.');
      return dotPos != String::npos
        ? filename.substr(dotPos + 1)
        : String();
    }

    virtual String Subpath() const
    {
      return SubpathValue;
    }

    virtual Ptr WithSubpath(const String& subpath) const
    {
      return boost::make_shared<FileIdentifier>(PathValue, subpath);
    }
  private:
    String Serialize() const
    {
      //do not place scheme
      String res = PathValue;
      if (!SubpathValue.empty())
      {
        res += SUBPATH_DELIMITER;
        res += SubpathValue;
      }
      return res;
    }
  private:
    const String PathValue;
    const String SubpathValue;
    const String FullValue;
  };

  class MemoryMappedData : public Binary::Data
  {
  public:
    explicit MemoryMappedData(const std::string& path)
    try
      : File(path.c_str(), boost::interprocess::read_only)
      , Region(File, boost::interprocess::read_only)
    {
    }
    catch (const boost::interprocess::interprocess_exception& e)
    {
      throw Error(THIS_LINE, FromStdString(e.what()));
    }

    virtual const void* Start() const
    {
      return Region.get_address();
    }

    virtual std::size_t Size() const
    {
      return Region.get_size();
    }
  private:
    const boost::interprocess::file_mapping File;
    const boost::interprocess::mapped_region Region;
  };

  Binary::Data::Ptr OpenMemoryMappedFile(const std::string& path)
  {
    return boost::make_shared<MemoryMappedData>(path);
  }

  Binary::Data::Ptr ReadFileToMemory(std::ifstream& stream, std::size_t size)
  {
    std::auto_ptr<Dump> res(new Dump(size));
    const std::streampos read = stream.read(safe_ptr_cast<char*>(&res->front()), size).tellg();
    if (static_cast<std::size_t>(read) != size)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to read %1% bytes. Actually got %2% bytes."), size, read);
    }
    //TODO: Binary::CreateData
    return Binary::CreateContainer(res);
  }

  //since dingux platform does not support wide strings(???) that boost.filesystem v3 requires, specify adapters in return-style
  boost::uintmax_t FileSize(const boost::filesystem::path& filePath, Error::LocationRef loc)
  {
    try
    {
      return boost::filesystem::file_size(filePath);
    }
    catch (const boost::system::system_error& err)
    {
      throw Error(loc, ToStdString(err.code().message()));
    }
  }
  
  bool IsDirectory(const boost::filesystem::path& filePath)
  {
    try
    {
      return boost::filesystem::is_directory(filePath);
    }
    catch (const boost::system::system_error&)
    {
      return false;
    }
  }
  
  bool IsExists(const boost::filesystem::path& filePath)
  {
    try
    {
      return boost::filesystem::exists(filePath);
    }
    catch (const boost::system::system_error&)
    {
      return false;
    }
  }
  
  void CreateDirectory(const boost::filesystem::path& path, Error::LocationRef loc)
  {
    try
    {
      //do not check result
      boost::filesystem::create_directory(path);
    }
    catch (const boost::system::system_error& err)
    {
      throw Error(loc, ToStdString(err.code().message()));
    }
  }

  Binary::Data::Ptr OpenFileData(const String& path, std::size_t mmapThreshold)
  {
    const boost::filesystem::path fileName(path);
    const boost::uintmax_t size = FileSize(fileName, THIS_LINE);
    if (size == 0)
    {
      throw Error(THIS_LINE, translate("File is empty."));
    }
    else if (size >= mmapThreshold)
    {
      Dbg("Using memory-mapped i/o for '%1%'.", path);
      return OpenMemoryMappedFile(fileName.string());
    }
    else
    {
      Dbg("Reading '%1%' to memory.", path);
      boost::filesystem::ifstream stream(fileName, std::ios::binary);
      return ReadFileToMemory(stream, static_cast<std::size_t>(size));
    }
  }

  class OutputFileStream : public Binary::SeekableOutputStream
  {
  public:
    explicit OutputFileStream(const boost::filesystem::path& name)
      : Name(name.string())
      , Stream(name, std::ios::binary)
    {
      if (!Stream)
      {
        throw Error(THIS_LINE, translate("Failed to open file."));
      }
    }

    virtual void ApplyData(const Binary::Data& data)
    {
      if (!Stream.write(static_cast<const char*>(data.Start()), data.Size()))
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to write file '%1%'"), Name);
      }
    }

    virtual void Flush()
    {
      if (!Stream.flush())
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to flush file '%1%'"), Name);
      }
    }

    virtual void Seek(uint64_t pos)
    {
      if (!Stream.seekp(pos))
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to seek file '%1%'"), Name);
      }
    }

    virtual uint64_t Position() const
    {
      return const_cast<boost::filesystem::ofstream&>(Stream).tellp();
    }
  private:
    const String Name;
    boost::filesystem::ofstream Stream;
  };

  //standard implementation does not work in mingw
  void CreateDirectoryRecursive(const boost::filesystem::path& dir)
  {
    if (IsDirectory(dir))
    {
      return;
    }
    const boost::filesystem::path& parent = dir.parent_path();
    if (!parent.empty())
    {
      CreateDirectoryRecursive(parent);
    }
    CreateDirectory(dir, THIS_LINE);
  }

  Binary::SeekableOutputStream::Ptr CreateFileStream(const String& fileName, const FileCreatingParameters& params)
  {
    const boost::filesystem::path path(fileName);
    if (params.CreateDirectories() && path.has_parent_path())
    {
      CreateDirectoryRecursive(path.parent_path());
    }
    if (!params.Overwrite() && IsExists(path))
    {
      throw Error(THIS_LINE, translate("File already exists."));
    }
    return boost::make_shared<OutputFileStream>(path);
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
      return translate("Local files and file:// scheme support");
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Strings::Set Schemes() const
    {
      static const Char* SCHEMES[] = 
      {
        SCHEME_FILE
      };
      return Strings::Set(SCHEMES, ArrayEnd(SCHEMES));
    }

    virtual Identifier::Ptr Resolve(const String& uri) const
    {
      const String::size_type schemePos = uri.find(SCHEME_SIGN);
      const String::size_type hierPos = String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1;
      const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER, hierPos);

      const String scheme = String::npos == schemePos ? String(SCHEME_FILE) : uri.substr(0, schemePos);
      const String path = String::npos == subPos ? uri.substr(hierPos) : uri.substr(hierPos, subPos - hierPos);
      const String subpath = String::npos == subPos ? String() : uri.substr(subPos + 1);
      return !path.empty() && scheme == SCHEME_FILE
        ? boost::make_shared<FileIdentifier>(path, subpath)
        : Identifier::Ptr();
    }

    virtual Binary::Container::Ptr Open(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& /*cb*/) const
    {
      const FileProviderParameters parameters(params);
      return Binary::CreateContainer(OpenLocalFile(path, parameters.MemoryMappingThreshold()));
    }
  };
}

namespace ZXTune
{
  namespace IO
  {
    Binary::Data::Ptr OpenLocalFile(const String& path, std::size_t mmapThreshold)
    {
      try
      {
        return OpenFileData(path, mmapThreshold);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to open file '%1%'."), path).AddSuberror(e);
      }
    }

    Binary::SeekableOutputStream::Ptr CreateLocalFile(const String& path, const FileCreatingParameters& params)
    {
      try
      {
        return CreateFileStream(path, params);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to create file '%1%'."), path).AddSuberror(e);
      }
    }

    DataProvider::Ptr CreateFileDataProvider()
    {
      return boost::make_shared<FileDataProvider>();
    }

    void RegisterFileProvider(ProvidersEnumerator& enumerator)
    {
      enumerator.RegisterProvider(CreateFileDataProvider());
    }
  }
}
