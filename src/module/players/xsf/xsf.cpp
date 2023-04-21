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
#include <strings/split.h>
// boost includes
#include <boost/algorithm/string/join.hpp>

namespace Module
{
  namespace XSF
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
        Strings::Split(str, "/\\"_sv, Components);
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
        static const String DELIMITER(1, '/');
        return boost::algorithm::join(Components, DELIMITER);
      }

    private:
      Strings::Array Components;
    };

    class FileBuilder : public Formats::Chiptune::PortableSoundFormat::Builder
    {
    public:
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

      void SetTitle(String title) override
      {
        GetMeta().Title = std::move(title);
      }

      void SetArtist(String artist) override
      {
        GetMeta().Artist = std::move(artist);
      }

      void SetGame(String game) override
      {
        GetMeta().Game = std::move(game);
      }

      void SetYear(String date) override
      {
        GetMeta().Year = std::move(date);
      }

      void SetGenre(String genre) override
      {
        GetMeta().Genre = std::move(genre);
      }

      void SetComment(String comment) override
      {
        GetMeta().Comment = std::move(comment);
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
        if (name == "_refresh")
        {
          GetMeta().RefreshRate = std::atoi(value.c_str());
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
  }  // namespace XSF
}  // namespace Module
