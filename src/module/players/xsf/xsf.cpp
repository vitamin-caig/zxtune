/**
 *
 * @file
 *
 * @brief  Xsf-based files structure support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/xsf.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <formats/chiptune/emulation/portablesoundformat.h>
#include <strings/conversion.h>
#include <strings/join.h>
#include <strings/split.h>

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
      const auto& elements = Strings::Split(str, R"(/\)"_sv);
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
      return Strings::Join(Components, "/"_sv);
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

    void SetYear(StringView date) override
    {
      GetMeta().Year = date;
    }

    void SetGenre(StringView genre) override
    {
      GetMeta().Genre = genre;
    }

    void SetCopyright(StringView copyright) override
    {
      GetMeta().Copyright = copyright;
    }

    void SetDumper(StringView dumper) override
    {
      GetMeta().Dumper = dumper;
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

    void SetTag(StringView name, StringView value) override
    {
      if (name == "_refresh"_sv)
      {
        GetMeta().RefreshRate = Strings::ConvertTo<uint_t>(value);
      }
      else
      {
        GetMeta().Tags.emplace_back(name, value);
      }
    }

    void SetLibrary(uint_t num, StringView filename) override
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
}  // namespace Module::XSF
