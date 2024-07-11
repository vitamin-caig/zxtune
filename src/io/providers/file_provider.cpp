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
#include "io/impl/filesystem_path.h"
#include "io/impl/l10n.h"
#include "io/providers/enumerator.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <binary/data_builder.h>
#include <debug/log.h>
#include <io/providers_parameters.h>
#include <parameters/accessor.h>
#include <strings/encoding.h>
#include <strings/format.h>
#include <strings/trim.h>
// std includes
#include <cctype>
#include <fstream>
// boost includes
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#undef min

namespace
{
// TODO
#ifdef _WIN32
  String ApplyOSFilenamesRestrictions(String in)
  {
    static const StringView DEPRECATED_NAMES[] = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
                                                  "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
                                                  "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    const auto dotPos = in.find('.');
    const auto filename = in.substr(0, dotPos);
    if (std::end(DEPRECATED_NAMES) != std::find(DEPRECATED_NAMES, std::end(DEPRECATED_NAMES), filename))
    {
      const auto restPart = dotPos != in.npos ? in.substr(dotPos) : StringView();
      // TODO: Concat(StringView...)
      return String{filename} + '~' + restPart;
    }
    return in;
  }

#else
  String ApplyOSFilenamesRestrictions(String in)
  {
    return in;
  }
#endif

  String GetErrorMessage(const std::filesystem::filesystem_error& err)
  {
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
      using namespace Parameters::ZXTune::IO::Providers::File;
      return Parameters::GetInteger<std::size_t>(Accessor, MMAP_THRESHOLD, MMAP_THRESHOLD_DEFAULT);
    }

    OverwriteMode Overwrite() const override
    {
      using namespace Parameters::ZXTune::IO::Providers::File;
      return Parameters::GetInteger<OverwriteMode>(Accessor, OVERWRITE_EXISTING, OVERWRITE_EXISTING_DEFAULT);
    }

    bool CreateDirectories() const override
    {
      using namespace Parameters::ZXTune::IO::Providers::File;
      return 0 != Parameters::GetInteger(Accessor, CREATE_DIRECTORIES, CREATE_DIRECTORIES_DEFAULT);
    }

    bool SanitizeNames() const override
    {
      using namespace Parameters::ZXTune::IO::Providers::File;
      return 0 != Parameters::GetInteger(Accessor, SANITIZE_NAMES, SANITIZE_NAMES_DEFAULT);
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  // uri-related constants
  const auto SCHEME_SIGN = "://"_sv;
  const auto SCHEME_FILE = "file"_sv;
  const Char SUBPATH_DELIMITER = '\?';

  class FileIdentifier : public Identifier
  {
  public:
    FileIdentifier(std::filesystem::path path, StringView subpath)
      : PathValue(std::move(path))
      , SubpathValue(subpath)
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
      return String{SCHEME_FILE};
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

    Ptr WithSubpath(StringView subpath) const override
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
    const std::filesystem::path PathValue;
    const String SubpathValue;
    const String FullValue;
  };

  class MemoryMappedData : public Binary::Data
  {
  public:
    explicit MemoryMappedData(const std::filesystem::path& path)
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

  Binary::Data::Ptr OpenMemoryMappedFile(const std::filesystem::path& path)
  {
    return MakePtr<MemoryMappedData>(path);
  }

  Binary::Data::Ptr ReadFileToMemory(std::ifstream& stream, std::size_t size)
  {
    Binary::DataBuilder res(size);
    const auto read = stream.read(static_cast<char*>(res.Allocate(size)), size).tellg();
    if (static_cast<std::size_t>(read) != size)
    {
      throw MakeFormattedError(THIS_LINE, translate("Failed to read {0} bytes. Actually got {1} bytes."), size, read);
    }
    return res.CaptureResult();
  }

  // since dingux platform does not support wide strings(???) that boost.filesystem v3 requires, specify adapters in
  // return-style
  std::uintmax_t FileSize(const std::filesystem::path& filePath, Error::LocationRef loc)
  {
    try
    {
      return std::filesystem::file_size(filePath);
    }
    catch (const std::filesystem::filesystem_error& err)
    {
      throw Error(loc, GetErrorMessage(err));
    }
  }

  bool IsDirectory(const std::filesystem::path& filePath)
  {
    try
    {
      return std::filesystem::is_directory(filePath);
    }
    catch (const std::filesystem::filesystem_error&)
    {
      return false;
    }
  }

  bool IsExists(const std::filesystem::path& filePath)
  {
    try
    {
      return std::filesystem::exists(filePath);
    }
    catch (const std::filesystem::filesystem_error&)
    {
      return false;
    }
  }

  void CreateDirectoryRecursive(const std::filesystem::path& path, Error::LocationRef loc)
  {
    try
    {
      // do not check result
      std::filesystem::create_directories(path);
    }
    catch (const std::filesystem::filesystem_error& err)
    {
      throw Error(loc, GetErrorMessage(err));
    }
  }

  Binary::Data::Ptr OpenData(StringView path, std::size_t mmapThreshold)
  {
    const auto fileName = Details::FromString(path);
    const auto size = FileSize(fileName, THIS_LINE);
    if (size == 0)
    {
      throw Error(THIS_LINE, translate("File is empty."));
    }
    else if (size >= mmapThreshold)
    {
      Dbg("Using memory-mapped i/o for '{}'.", path);
      // use local encoding here
      return OpenMemoryMappedFile(fileName);
    }
    else
    {
      Dbg("Reading '{}' to memory.", path);
      std::ifstream stream(fileName, std::ios::binary);
      return ReadFileToMemory(stream, static_cast<std::size_t>(size));
    }
  }

  class OutputFileStream : public Binary::SeekableOutputStream
  {
  public:
    explicit OutputFileStream(const std::filesystem::path& name)
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
      return const_cast<std::ofstream&>(Stream).tellp();
    }

  private:
    const String Name;
    std::ofstream Stream;
  };

  String SanitizePathComponent(String input)
  {
    input = Strings::Trim(input, &IsNotFSSymbol);
    std::replace_if(input.begin(), input.end(), &IsNotFSSymbol, Char('_'));
    return ApplyOSFilenamesRestrictions(std::move(input));
  }

  std::filesystem::path CreateSanitizedPath(StringView fileName)
  {
    const auto initial = Details::FromString(fileName);
    auto it = initial.begin();
    auto lim = initial.end();
    std::filesystem::path result;
    for (const auto root = initial.root_path(); result != root && it != lim; ++it)
    {
      result /= *it;
    }
    for (; it != lim; ++it)
    {
      const auto sanitized = Details::FromString(SanitizePathComponent(Details::ToString(*it)));
      result /= sanitized;
    }
    if (initial != result)
    {
      Dbg("Sanitized path '{}' to '{}'", fileName, Details::ToString(result));
    }
    return result;
  }

  Binary::SeekableOutputStream::Ptr CreateStream(StringView fileName, const FileCreatingParameters& params)
  {
    try
    {
      auto path = params.SanitizeNames() ? CreateSanitizedPath(fileName) : Details::FromString(fileName);
      Dbg("CreateStream: input='{}' path='{}'", fileName, Details::ToString(path));
      if (params.CreateDirectories() && path.has_parent_path())
      {
        CreateDirectoryRecursive(path.parent_path(), THIS_LINE);
      }
      switch (params.Overwrite())
      {
      case OverwriteMode::STOP_IF_EXISTS:
        if (IsExists(path))
        {
          throw Error(THIS_LINE, translate("File already exists."));
        }
        break;
      case OverwriteMode::RENAME_NEW:
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
      case OverwriteMode::OVERWRITE_EXISTING:
        break;
      default:
        Require(false);
      }
      return MakePtr<OutputFileStream>(path);
    }
    catch (const std::filesystem::filesystem_error& err)
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
      return {};
    }

    Identifier::Ptr Resolve(StringView uri) const override
    {
      const auto schemePos = uri.find(SCHEME_SIGN);
      const auto hierPos = uri.npos == schemePos ? 0 : schemePos + SCHEME_SIGN.size();
      const auto subPos = uri.find_first_of(SUBPATH_DELIMITER, hierPos);

      const auto scheme = uri.npos == schemePos ? SCHEME_FILE : uri.substr(0, schemePos);
      const auto path = uri.npos == subPos ? uri.substr(hierPos) : uri.substr(hierPos, subPos - hierPos);
      const auto subpath = uri.npos == subPos ? StringView() : uri.substr(subPos + 1);
      return !path.empty() && scheme == SCHEME_FILE ? MakePtr<FileIdentifier>(Details::FromString(path), subpath)
                                                    : Identifier::Ptr();
    }

    Binary::Container::Ptr Open(StringView path, const Parameters::Accessor& params,
                                Log::ProgressCallback& /*cb*/) const override
    {
      const ProviderParameters parameters(params);
      return Binary::CreateContainer(OpenLocalFile(path, parameters.MemoryMappingThreshold()));
    }

    Binary::OutputStream::Ptr Create(StringView path, const Parameters::Accessor& params,
                                     Log::ProgressCallback&) const override
    {
      const ProviderParameters parameters(params);
      return CreateLocalFile(path, parameters);
    }
  };
}  // namespace IO::File

namespace IO
{
  Binary::Data::Ptr OpenLocalFile(StringView path, std::size_t mmapThreshold)
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

  Binary::SeekableOutputStream::Ptr CreateLocalFile(StringView path, const FileCreatingParameters& params)
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
