/**
 *
 * @file
 *
 * @brief  File-based backends implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/file_backend.h"
#include "sound/backends/l10n.h"
// common includes
#include <make_ptr.h>
#include <progress_callback.h>
// library includes
#include <async/data_receiver.h>
#include <debug/log.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <io/template.h>
#include <module/attributes.h>
#include <module/track_state.h>
#include <parameters/convert.h>
#include <parameters/template.h>
#include <sound/backends_parameters.h>
// std includes
#include <memory>
#include <utility>

namespace Sound::File
{
  const Debug::Stream Dbg("Sound::Backend::FileBase");

  const Char DEFAULT_COMMENT[] = "Created using ZXTune toolkit";

  class StateFieldsSource : public Strings::SkipFieldsSource
  {
  public:
    explicit StateFieldsSource(const Module::TrackState& state)
      : State(state)
    {}

    String GetFieldValue(StringView fieldName) const override
    {
      if (fieldName == Module::ATTR_CURRENT_POSITION)
      {
        return Parameters::ConvertToString(State.Position());
      }
      else if (fieldName == Module::ATTR_CURRENT_PATTERN)
      {
        return Parameters::ConvertToString(State.Pattern());
      }
      else if (fieldName == Module::ATTR_CURRENT_LINE)
      {
        return Parameters::ConvertToString(State.Line());
      }
      return Strings::SkipFieldsSource::GetFieldValue(fieldName);
    }

  private:
    const Module::TrackState& State;
  };

  class TrackStateTemplate
  {
  public:
    explicit TrackStateTemplate(StringView templ)
      : Template(Strings::Template::Create(templ))
      , CurPosition(HasField(templ, Module::ATTR_CURRENT_POSITION))
      , CurPattern(HasField(templ, Module::ATTR_CURRENT_PATTERN))
      , CurLine(HasField(templ, Module::ATTR_CURRENT_LINE))
      , Result(Template->Instantiate(Strings::SkipFieldsSource()))
    {}

    String Instantiate(const Module::State& state) const
    {
      if (const auto* const track = dynamic_cast<const Module::TrackState*>(&state))
      {
        if (CurPosition.Update(track->Position()) || CurPattern.Update(track->Pattern())
            || CurLine.Update(track->Line()))
        {
          const StateFieldsSource source(*track);
          Result = Template->Instantiate(source);
        }
      }
      return Result;
    }

  private:
    static bool HasField(StringView templ, StringView name)
    {
      // TODO: Concat(StringView...)
      const String fullName = String{Strings::Template::FIELD_START} + name + Strings::Template::FIELD_END;
      return String::npos != templ.find(fullName);
    }

  private:
    class TrackableValue
    {
    public:
      explicit TrackableValue(bool trackable)
        : Trackable(trackable)
      {}

      bool Update(int_t newVal)
      {
        if (Trackable && Value != newVal)
        {
          Value = newVal;
          return true;
        }
        return false;
      }

    private:
      const bool Trackable;
      int_t Value = -1;
    };

  private:
    const Strings::Template::Ptr Template;
    mutable TrackableValue CurPosition;
    mutable TrackableValue CurPattern;
    mutable TrackableValue CurLine;
    mutable String Result;
  };

  class FileParameters
  {
  public:
    FileParameters(Parameters::Accessor::Ptr params, BackendId id)
      : Params(std::move(params))
      , Id(id)
    {}

    String GetFilenameTemplate() const
    {
      auto nameTemplate = GetProperty<Parameters::StringType>(Parameters::ZXTune::Sound::Backends::File::FILENAME);
      if (nameTemplate.empty())
      {
        // Filename parameter is required
        throw Error(THIS_LINE, translate("Output filename template is not specified."));
      }
      // check if required to add extension
      const String extension = String(1, '.').append(Id);
      const String::size_type extPos = nameTemplate.find(extension);
      if (String::npos == extPos || extPos + extension.size() != nameTemplate.size())
      {
        nameTemplate += extension;
      }
      return nameTemplate;
    }

    uint_t GetBuffersCount() const
    {
      const auto intParam = GetProperty<Parameters::IntType>(Parameters::ZXTune::Sound::Backends::File::BUFFERS);
      return static_cast<uint_t>(intParam);
    }

  private:
    template<class T>
    T GetProperty(Parameters::Identifier property) const
    {
      T result = T();
      if (!Parameters::FindValue(*Params, ReplaceBackendId(property), result))
      {
        Parameters::FindValue(*Params, property, result);
      }
      return result;
    }

    String ReplaceBackendId(StringView property) const
    {
      // TODO: think about better solution
      static const auto GENERIC_ID = ".file."_sv;
      const auto pos = property.find(GENERIC_ID);
      Require(pos != property.npos);
      auto result = String{property};
      result.replace(pos + 1, GENERIC_ID.size() - 2, Id);
      return result;
    }

  private:
    const Parameters::Accessor::Ptr Params;
    const BackendId Id;
  };

  String InstantiateModuleFields(StringView nameTemplate, const Parameters::Accessor& props)
  {
    Dbg("Original filename template: '{}'", nameTemplate);
    const Parameters::FieldsSourceAdapter<Strings::KeepFieldsSource> moduleFields(props);
    const auto templ = IO::CreateFilenameTemplate(nameTemplate);
    auto nameTemplateWithRuntimeFields = templ->Instantiate(moduleFields);
    Dbg("Fixed filename template: '{}'", nameTemplateWithRuntimeFields);
    return nameTemplateWithRuntimeFields;
  }

  class StreamSource
  {
  public:
    StreamSource(Parameters::Accessor::Ptr params, Parameters::Accessor::Ptr properties, FileStreamFactory::Ptr factory)
      : Params(std::move(params))
      , Properties(std::move(properties))
      , Factory(std::move(factory))
      , FileParams(Params, Factory->GetId())
      , FilenameTemplate(InstantiateModuleFields(FileParams.GetFilenameTemplate(), *Properties))
    {}

    Receiver::Ptr GetStream(const Module::State& state) const
    {
      const auto& newFilename = FilenameTemplate.Instantiate(state);
      if (Filename != newFilename)
      {
        auto stream = IO::CreateStream(newFilename, *Params, Log::ProgressCallback::Stub());
        Filename = newFilename;
        auto result = Factory->CreateStream(std::move(stream));
        SetProperties(*result);
        if (const uint_t buffers = FileParams.GetBuffersCount())
        {
          return Async::DataReceiver<Chunk>::Create(1, buffers, std::move(result));
        }
        else
        {
          return result;
        }
      }
      return {};
    }

  private:
    void SetProperties(FileStream& stream) const
    {
      if (const auto str = Parameters::GetString(*Properties, Module::ATTR_TITLE); !str.empty())
      {
        stream.SetTitle(str);
      }
      if (const auto str = Parameters::GetString(*Properties, Module::ATTR_AUTHOR); !str.empty())
      {
        stream.SetAuthor(str);
      }
      if (const auto str = Parameters::GetString(*Properties, Module::ATTR_COMMENT); !str.empty())
      {
        stream.SetComment(str);
      }
      else
      {
        stream.SetComment(DEFAULT_COMMENT);
      }
      stream.FlushMetadata();
    }

  private:
    const Parameters::Accessor::Ptr Params;
    const Parameters::Accessor::Ptr Properties;
    const FileStreamFactory::Ptr Factory;
    const FileParameters FileParams;
    const TrackStateTemplate FilenameTemplate;
    mutable String Filename;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Parameters::Accessor::Ptr params, Parameters::Accessor::Ptr properties,
                  FileStreamFactory::Ptr factory)
      : Params(std::move(params))
      , Properties(std::move(properties))
      , Factory(std::move(factory))
      , Stream(Receiver::CreateStub())
    {}

    // BackendWorker
    void Startup() override
    {
      Source = std::make_unique<StreamSource>(Params, Properties, Factory);
    }

    void Shutdown() override
    {
      SetStream(Receiver::CreateStub());
      Source.reset();
    }

    void Pause() override {}

    void Resume() override {}

    void FrameStart(const Module::State& state) override
    {
      if (const Receiver::Ptr newStream = Source->GetStream(state))
      {
        SetStream(newStream);
      }
    }

    void FrameFinish(Chunk buffer) override
    {
      assert(Stream);
      Stream->ApplyData(std::move(buffer));
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      // Does not support volume control
      return {};
    }

  private:
    void SetStream(Receiver::Ptr str)
    {
      Stream->Flush();
      Stream = std::move(str);
    }

  private:
    const Parameters::Accessor::Ptr Params;
    const Parameters::Accessor::Ptr Properties;
    const FileStreamFactory::Ptr Factory;
    std::unique_ptr<StreamSource> Source;
    Receiver::Ptr Stream;
  };
}  // namespace Sound::File

namespace Sound
{
  BackendWorker::Ptr CreateFileBackendWorker(Parameters::Accessor::Ptr params, Parameters::Accessor::Ptr properties,
                                             FileStreamFactory::Ptr factory)
  {
    return MakePtr<File::BackendWorker>(std::move(params), std::move(properties), std::move(factory));
  }
}  // namespace Sound
