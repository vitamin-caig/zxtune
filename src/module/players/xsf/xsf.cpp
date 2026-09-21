/**
 *
 * @file
 *
 * @brief  Xsf-based files structure support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/xsf/xsf.h"

#include "formats/chiptune/emulation/portablesoundformat.h"

#include "strings/casing.h"
#include "strings/conversion.h"
#include "strings/join.h"
#include "strings/split.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

namespace Module::XSF
{
  class FilePath
  {
  private:
    explicit FilePath(Strings::Array&& rh) noexcept
      : Components(std::move(rh))
    {}

  public:
    explicit FilePath(StringView str)
    {
      const auto& elements = Strings::Split(str, R"(/\)"sv);
      Components.assign(elements.begin(), elements.end());
    }

    FilePath RelativeTo(const FilePath& rh) const
    {
      static const String PARENT_PATH("..");
      if (rh.Components.empty())
      {
        return *this;
      }
      Require(!Components.empty());
      auto newComponents = rh.Components;
      newComponents.pop_back();
      auto it = Components.begin();
      const auto lim = Components.end();
      while (it != lim && *it == PARENT_PATH)
      {
        if (newComponents.empty() || newComponents.back() == PARENT_PATH)
        {
          break;
        }
        newComponents.pop_back();
      }
      std::copy(it, lim, std::back_inserter(newComponents));
      return FilePath(std::move(newComponents));
    }

    String ToString() const
    {
      return Strings::Join(Components, "/"sv);
    }

  private:
    Strings::Array Components;
  };

  class FileBuilder
    : public Formats::Chiptune::PortableSoundFormat::Builder
    , private Formats::Chiptune::MetaBuilder
  {
  public:
    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    void SetVersion(uint_t ver) override
    {
      Result.Version = ver;
    }

    void SetReservedSection(Binary::Container::Ptr blob) override
    {
      Result.ReservedSection = std::move(blob);
    }

    void SetPackedProgramSection(Binary::Container::Ptr blob) override
    {
      Result.PackedProgramSection = std::move(blob);
    }

    void SetYear(String date) override
    {
      GetMeta().Year = std::move(date);
    }

    void SetGenre(String genre) override
    {
      GetMeta().Genre = std::move(genre);
    }

    void SetCopyright(String copyright) override
    {
      GetMeta().Copyright = std::move(copyright);
    }

    void SetDumper(String dumper) override
    {
      GetMeta().Dumper = std::move(dumper);
    }

    void SetLength(Time::Milliseconds duration) override
    {
      GetMeta().Duration = duration;
    }

    void SetFade(Time::Milliseconds fade) override
    {
      GetMeta().Fadeout = fade;
    }

    void SetVolume(float vol) override
    {
      GetMeta().Volume = vol;
    }

    void SetTag(String name, String value) override
    {
      if (name == "_refresh"sv)
      {
        GetMeta().RefreshRate = Strings::ConvertTo<uint_t>(value);
      }
      else
      {
        GetMeta().Tags.emplace_back(std::move(name), std::move(value));
      }
    }

    void SetLibrary(uint_t num, String filename) override
    {
      Result.Dependencies.resize(std::max<std::size_t>(Result.Dependencies.size(), num));
      Result.Dependencies[num - 1] = FilePath(filename).ToString();
    }

    // MetaBuilder
    void SetProgram(StringView program) override
    {
      GetMeta().Game = program;
    }

    void SetTitle(StringView title) override
    {
      GetMeta().Title = title;
    }

    void SetAuthor(StringView author) override
    {
      GetMeta().Artist = author;
    }
    void SetStrings(const Strings::Array& /*strings*/) override {}
    void SetComment(StringView comment) override
    {
      GetMeta().Comment = comment;
    }

    void MakeDependenciesRelativeTo(StringView filename)
    {
      const FilePath root(filename);
      for (auto& dep : Result.Dependencies)
      {
        const FilePath depPath(dep);
        dep = depPath.RelativeTo(root).ToString();
      }
    }

    File CaptureResult()
    {
      return std::move(Result);
    }

  private:
    MetaInformation& GetMeta()
    {
      if (!Meta)
      {
        Result.Meta.reset(Meta = new MetaInformation());
      }
      return *Meta;
    }

  private:
    File Result;
    MetaInformation* Meta = nullptr;
  };

  Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, File& file)
  {
    FileBuilder builder;
    if (auto source = Formats::Chiptune::PortableSoundFormat::Parse(rawData, builder))
    {
      file = builder.CaptureResult();
      return source;
    }
    else
    {
      return {};
    }
  }

  Formats::Chiptune::Container::Ptr Parse(StringView name, const Binary::Container& rawData, File& file)
  {
    FileBuilder builder;
    if (auto source = Formats::Chiptune::PortableSoundFormat::Parse(rawData, builder))
    {
      builder.MakeDependenciesRelativeTo(name);
      file = builder.CaptureResult();
      return source;
    }
    else
    {
      return {};
    }
  }

  const uint_t MAX_LEVEL = 10;

  /* https://gist.githubusercontent.com/SaxxonPike/a0b47f8579aad703b842001b24d40c00/raw/a6fa28b44fb598b8874923dbffe932459f6a61b9/psf_format.txt

  The proper way to load a minipsf is as follows:
  - Load the executable data from the minipsf - this becomes the current executable.
  - Check for the presence of a "_lib" tag. If present:
    - RECURSIVELY load the executable data from the given library file. (Make sure to limit recursion to avoid
  crashing - I usually limit it to 10 levels)
    - Make the _lib executable the current one.
    - If applicable, we will use the initial program counter/stack pointer from the _lib executable.
    - Superimpose the originally loaded minipsf executable on top of the current executable. If applicable, use the
  start address and size to determine where to .
  - Check for the presence of "_libN" tags for N=2 and up (use "_lib%d")
    - RECURSIVELY load and superimpose all these EXEs on top of the current EXE. Do not modify the current program
  counter or stack pointer.
    - Start at N=2. Stop at the first tag name that doesn't exist.
  - (done)
  */

  // PSF, GSF and SSF/DSF formats follow this loading sequence for the program section
  void MergeProgramSectionsMultiLibrary(const File& data, const FilesMap& additionalFiles, MergeTarget& dst,
                                        uint_t level)
  {
    auto it = data.Dependencies.begin();
    const auto lim = data.Dependencies.end();
    if (it != lim && level < MAX_LEVEL)
    {
      MergeProgramSectionsMultiLibrary(additionalFiles.at(*it), additionalFiles, dst, level + 1);
    }
    dst.AddProgramSection(data.PackedProgramSection);
    if (it != lim && level < MAX_LEVEL)
    {
      for (++it; it != lim; ++it)
      {
        MergeProgramSectionsMultiLibrary(additionalFiles.at(*it), additionalFiles, dst, level + 1);
      }
    }
  }

  /* https://hcs64.com/usf/usf.txt

  Loading a USF or USFlib/miniUSF

  1. initialize the ROM and save state to zero.
  2. if the USF contains a _lib tag (_libn not supported as of this version)
     recursively load the specified file starting from step 2
  3. load the ROM and save state, replacing any data with the same addresses that
     may have already been loaded
  */

  // USF, NCSF and 2SF formats use a single library and follow this loading sequence, joining both of the
  // sections.
  // NCSF: https://www.cyberbotx.com/NCSF/
  // 2SF: https://web.archive.org/web/20150703023816if_/http://unknown.hcs64.com/2sf/2sfspec.txt
  void MergeSectionsSingleLibrary(const File& data, const FilesMap& additionalFiles, MergeTarget& dst, uint_t level)
  {
    if (!data.Dependencies.empty() && level < MAX_LEVEL)
    {
      MergeSectionsSingleLibrary(additionalFiles.at(data.Dependencies.front()), additionalFiles, dst, level + 1);
    }
    dst.AddProgramSection(data.PackedProgramSection);
    dst.AddReservedSection(data.ReservedSection);
  }

  /* https://web.archive.org/web/20110721002934/http://wiki.neillcorlett.com/MiniPSF2

  The proper way to load a MiniPSF2 is as follows:
  - First, recursively load the virtual filesystems from each PSF2 file named by a library tag.
    - The first tag is "_lib"
    - The remaining tags are "_libN" for N>=2 (use "_lib%d")
    - Stop at the first tag name that doesn't exist.
  - Then, load the virtual filesystem from the current PSF2 file.

  If there are conflicting or redundant filenames, they should be overwritten in memory in the order in which the
  filesystem data was parsed. Later takes priority.
  */

  // The own file's reserved section is applied last, so it overrides the previously loaded data
  void MergeReservedSectionsDepsFirst(const File& data, const FilesMap& additionalFiles, MergeTarget& dst, uint_t level)
  {
    if (level < MAX_LEVEL)
    {
      for (const auto& dep : data.Dependencies)
      {
        MergeReservedSectionsDepsFirst(additionalFiles.at(dep), additionalFiles, dst, level + 1);
      }
    }
    dst.AddReservedSection(data.ReservedSection);
  }

  // Meta information is merged the same way for all formats, the later files take priority
  void MergeMeta(const File& data, const FilesMap& additionalFiles, MergeTarget& dst, uint_t level)
  {
    if (level < MAX_LEVEL)
    {
      for (const auto& dep : data.Dependencies)
      {
        MergeMeta(additionalFiles.at(dep), additionalFiles, dst, level + 1);
      }
    }
    if (data.Meta)
    {
      dst.AddMeta(*data.Meta);
    }
  }
}  // namespace Module::XSF
