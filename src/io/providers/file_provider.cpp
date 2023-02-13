/**
 *
 * @file
 *
 * @brief  File provider implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "io/providers/file_provider.h"
#include "io/impl/boost_filesystem_path.h"
#include "io/impl/l10n.h"
#include "io/providers/enumerator.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <binary/container_factories.h>
#include <debug/log.h>
#include <io/providers_parameters.h>
#include <parameters/accessor.h>
#include <strings/encoding.h>
#include <strings/format.h>
// std includes
#include <cctype>
// boost includes
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#undef min

namespace
{
// TODO
#ifdef _WIN32
  String ApplyOSFilenamesRestrictions(const String& in)
  {
    static const String DEPRECATED_NAMES[] = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
                                              "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
                                              "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    const auto dotPos = in.find('.');
    const auto filename = in.substr(0, dotPos);
    if (std::end(DEPRECATED_NAMES) != std::find(DEPRECATED_NAMES, std::end(DEPRECATED_NAMES), filename))
    {
      const auto restPart = dotPos != String::npos ? in.substr(dotPos) : String();
      return filename + '~' + restPart;
    }
    return in;
  }

#else
  String ApplyOSFilenamesRestrictions(const String& in)
  {
    return in;
  }
#endif

  String GetErrorMessage(const boost::system::system_error& err)
  {
    // TODO: remove when BOOST_NO_ANSI_APIS will be applied
    return Strings::ToAutoUtf8(err.code().message());
  }

  inline bool IsNotFSSymbol(Char sym)
  {
    return std::iscntrl(sym) || sym == '*' || sym == '\?' || sym == '%' || sym == ':' || sym == '|' || sym == '\"'
           || sym == '<' || sym == '>' || sym == '\\' || sym == '/';
  }
}  // namespace

namespace IO::File
{
  const Debug::Stream Dbg("IO::Provider::File");

  class ProviderParameters : public FileCreatingParameters
  {
  public:
    explicit ProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    std::size_t MemoryMappingThreshold() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, intVal);
      return static_cast<std::size_t>(intVal);
    }

    OverwriteMode Overwrite() const override
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, intVal);
      return static_cast<OverwriteMode>(intVal);
    }

    bool CreateDirectories() const override
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, intVal);
      return intVal != 0;
    }

    bool SanitizeNames() const override
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::SANITIZE_NAMES_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::SANITIZE_NAMES, intVal);
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
    FileIdentifier(boost::filesystem::path path, String subpath)
      : PathValue(std::move(path))
      , SubpathValue(std::move(subpath))
      , FullValue(Serialize())
    {
      Require(!PathValue.empty());
    }

    String Full() const override
    {
      return FullValue;
    }

    String Scheme() const override
    {
      return SCHEME_FILE;
    }

    String Path() const override
    {
      return Details::ToString(PathValue);
    }

    String Filename() const override
    {
      return Details::ToString(PathValue.filename());
    }

    String Extension() const override
    {
      const String result = Details::ToString(PathValue.extension());
      return result.empty() ? result : result.substr(1);  // skip initial dot
    }

    String Subpath() const override
    {
      return SubpathValue;
    }

    Ptr WithSubpath(const String& subpath) const override
    {
      return MakePtr<FileIdentifier>(PathValue, subpath);
    }

  private:
    String Serialize() const
    {
      // do not place scheme
      auto res = Details::ToString(PathValue);
      if (!SubpathValue.empty())
      {
        res += SUBPATH_DELIMITER;
        res += SubpathValue;
      }
      return res;
    }

  private:
    const boost::filesystem::path PathValue;
    const String SubpathValue;
    const String FullValue;
  };

  class MemoryMappedData : public Binary::Data
  {
  public:
    explicit MemoryMappedData(const String& path)
    try : File(path.c_str(), boost::interprocess::read_only), Region(File, boost::interprocess::read_only)
    {}
    catch (const boost::interprocess::interprocess_exception& e)
    {
      throw Error(THIS_LINE, e.what());
    }

    const void* Start() const override
    {
      return Region.get_address();
    }

    std::size_t Size() const override
    {
      return Region.get_size();
    }

  private:
    const boost::interprocess::file_mapping File;
    const boost::interprocess::mapped_region Region;
  };

  Binary::Data::Ptr OpenMemoryMappedFile(const String& path)
  {
    return MakePtr<MemoryMappedData>(path);
  }

  Binary::Data::Ptr ReadFileToMemory(std::ifstream& stream, std::size_t size)
  {
    std::unique_ptr<Binary::Dump> res(new Binary::Dump(size));
    const std::streampos read = stream.read(safe_ptr_cast<char*>(res->data()), size).tellg();
    if (static_cast<std::size_t>(read) != size)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to read {0} bytes. Actually got {1} bytes."), size, read);
    }
    // TODO: Binary::CreateData
    return Binary::CreateContainer(std::move(res));
  }

  // since dingux platform does not support wide strings(???) that boost.filesystem v3 requires, specify adapters in
  // return-style
  boost::uintmax_t FileSize(const boost::filesystem::path& filePath, Error::LocationRef loc)
  {
    try
    {
      return boost::filesystem::file_size(filePath);
    }
    catch (const boost::system::system_error& err)
    {
      throw Error(loc, GetErrorMessage(err));
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
      // do not check result
      boost::filesystem::create_directory(path);
    }
    catch (const boost::system::system_error& err)
    {
      throw Error(loc, GetErrorMessage(err));
    }
  }

  Binary::Data::Ptr OpenData(const String& path, std::size_t mmapThreshold)
  {
    const boost::filesystem::path fileName = Details::FromString(path);
    const boost::uintmax_t size = FileSize(fileName, THIS_LINE);
    if (size == 0)
    {
      throw Error(THIS_LINE, translate("File is empty."));
    }
    else if (size >= mmapThreshold)
    {
      Dbg("Using memory-mapped i/o for '{}'.", path);
      // use local encoding here
      return OpenMemoryMappedFile(fileName.string());
    }
    else
    {
      Dbg("Reading '{}' to memory.", path);
      boost::filesystem::ifstream stream(fileName, std::ios::binary);
      return ReadFileToMemory(stream, static_cast<std::size_t>(size));
    }
  }

  class OutputFileStream : public Binary::SeekableOutputStream
  {
  public:
    explicit OutputFileStream(const boost::filesystem::path& name)
      : Name(Details::ToString(name))
      , Stream(name, std::ios::binary | std::ios_base::out)
    {
      if (!Stream)
      {
        throw Error(THIS_LINE, translate("Failed to open file."));
      }
    }

    void ApplyData(Binary::View data) override
    {
      if (!Stream.write(static_cast<const char*>(data.Start()), data.Size()))
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to write file '{}'"), Name);
      }
    }

    void Flush() override
    {
      if (!Stream.flush())
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to flush file '{}'"), Name);
      }
    }

    void Seek(uint64_t pos) override
    {
      if (!Stream.seekp(pos))
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to seek file '{}'"), Name);
      }
    }

    uint64_t Position() const override
    {
      return const_cast<boost::filesystem::ofstream&>(Stream).tellp();
    }

  private:
    const String Name;
    boost::filesystem::ofstream Stream;
  };

  // standard implementation does not work in mingw
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

  String SanitizePathComponent(const String& input)
  {
    String result = boost::algorithm::trim_copy_if(input, &IsNotFSSymbol);
    std::replace_if(result.begin(), result.end(), &IsNotFSSymbol, Char('_'));
    return ApplyOSFilenamesRestrictions(result);
  }

  boost::filesystem::path CreateSanitizedPath(const String& fileName)
  {
    const boost::filesystem::path initial = Details::FromString(fileName);
    boost::filesystem::path::const_iterator it = initial.begin(), lim = initial.end();
    boost::filesystem::path result;
    for (const boost::filesystem::path root(initial.root_path()); result != root && it != lim; ++it)
    {
      result /= *it;
    }
    for (; it != lim; ++it)
    {
      const boost::filesystem::path sanitized = Details::FromString(SanitizePathComponent(Details::ToString(*it)));
      result /= sanitized;
    }
    if (initial != result)
    {
      Dbg("Sanitized path '{}' to '{}'", fileName, Details::ToString(result));
    }
    return result;
  }

  Binary::SeekableOutputStream::Ptr CreateStream(const String& fileName, const FileCreatingParameters& params)
  {
    try
    {
      boost::filesystem::path path = params.SanitizeNames() ? CreateSanitizedPath(fileName)
                                                            : Details::FromString(fileName);
      Dbg("CreateStream: input='{}' path='{}'", fileName, Details::ToString(path));
      if (params.CreateDirectories() && path.has_parent_path())
      {
        CreateDirectoryRecursive(path.parent_path());
      }
      switch (params.Overwrite())
      {
      case STOP_IF_EXISTS:
        if (IsExists(path))
        {
          throw Error(THIS_LINE, translate("File already exists."));
        }
        break;
      case RENAME_NEW:
      {
        const auto oldStem = path.stem();
        const auto extension = path.extension();
        for (uint_t idx = 1; IsExists(path); ++idx)
        {
          auto newFilename = oldStem;
          newFilename += Strings::Format(" ({})", idx);
          newFilename += extension;
          path.remove_filename();
          path /= newFilename;
        }
      }
      case OVERWRITE_EXISTING:
        break;
      default:
        Require(false);
      }
      return MakePtr<OutputFileStream>(path);
    }
    catch (const boost::system::system_error& err)
    {
      throw Error(THIS_LINE, GetErrorMessage(err));
    }
  }

  ///////////////////////////////////////
  const Char IDENTIFIER[] = "file";

  class DataProvider : public IO::DataProvider
  {
  public:
    String Id() const override
    {
      return IDENTIFIER;
    }

    String Description() const override
    {
      return translate("Local files and file:// scheme support");
    }

    Error Status() const override
    {
      return Error();
    }

    Strings::Set Schemes() const override
    {
      static const Char* SCHEMES[] = {SCHEME_FILE};
      return Strings::Set(SCHEMES, std::end(SCHEMES));
    }

    Identifier::Ptr Resolve(const String& uri) const override
    {
      const String schemeSign(SCHEME_SIGN);
      const String::size_type schemePos = uri.find(schemeSign);
      const String::size_type hierPos = String::npos == schemePos ? 0 : schemePos + schemeSign.size();
      const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER, hierPos);

      const String scheme = String::npos == schemePos ? String(SCHEME_FILE) : uri.substr(0, schemePos);
      const String path = String::npos == subPos ? uri.substr(hierPos) : uri.substr(hierPos, subPos - hierPos);
      const String subpath = String::npos == subPos ? String() : uri.substr(subPos + 1);
      return !path.empty() && scheme == SCHEME_FILE ? MakePtr<FileIdentifier>(Details::FromString(path), subpath)
                                                    : Identifier::Ptr();
    }

    Binary::Container::Ptr Open(const String& path, const Parameters::Accessor& params,
                                Log::ProgressCallback& /*cb*/) const override
    {
      const ProviderParameters parameters(params);
      return Binary::CreateContainer(OpenLocalFile(path, parameters.MemoryMappingThreshold()));
    }

    Binary::OutputStream::Ptr Create(const String& path, const Parameters::Accessor& params,
                                     Log::ProgressCallback&) const override
    {
      const ProviderParameters parameters(params);
      return CreateLocalFile(path, parameters);
    }
  };
}  // namespace IO::File

namespace IO
{
  Binary::Data::Ptr OpenLocalFile(const String& path, std::size_t mmapThreshold)
  {
    try
    {
      return File::OpenData(path, mmapThreshold);
    }
    catch (const Error& e)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to open file '{}'."), path).AddSuberror(e);
    }
  }

  Binary::SeekableOutputStream::Ptr CreateLocalFile(const String& path, const FileCreatingParameters& params)
  {
    try
    {
      return File::CreateStream(path, params);
    }
    catch (const Error& e)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to create file '{}'."), path).AddSuberror(e);
    }
  }

  DataProvider::Ptr CreateFileDataProvider()
  {
    return MakePtr<File::DataProvider>();
  }

  void RegisterFileProvider(ProvidersEnumerator& enumerator)
  {
    enumerator.RegisterProvider(CreateFileDataProvider());
  }
}  // namespace IO

