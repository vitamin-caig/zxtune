/**
*
* @file
*
* @brief  File-based backends implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "file_backend.h"
//common includes
#include <make_ptr.h>
#include <progress_callback.h>
//library includes
#include <async/data_receiver.h>
#include <debug/log.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <io/template.h>
#include <l10n/api.h>
#include <module/attributes.h>
#include <parameters/convert.h>
#include <parameters/template.h>
#include <sound/backends_parameters.h>
//text includes
#include "text/backends.h"

#define FILE_TAG B4CB6B0C

namespace
{
  const Debug::Stream Dbg("Sound::Backend::FileBase");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace File
{
  class StateFieldsSource : public Strings::SkipFieldsSource
  {
  public:
    explicit StateFieldsSource(const Module::TrackState& state)
      : State(state)
    {
    }

    String GetFieldValue(const String& fieldName) const override
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
    explicit TrackStateTemplate(const String& templ)
      : Template(Strings::Template::Create(templ))
      , CurPosition(HasField(templ, Module::ATTR_CURRENT_POSITION))
      , CurPattern(HasField(templ, Module::ATTR_CURRENT_PATTERN))
      , CurLine(HasField(templ, Module::ATTR_CURRENT_LINE))
      , Result(Template->Instantiate(Strings::SkipFieldsSource()))
    {
    }

    String Instantiate(const Module::TrackState& state) const
    {
      if (CurPosition.Update(state.Position()) ||
          CurPattern.Update(state.Pattern()) ||
          CurLine.Update(state.Line()))
      {
        const StateFieldsSource source(state);
        Result = Template->Instantiate(source);
      }
      return Result;
    }
  private:
    static bool HasField(const String& templ, const String& name)
    {
      const String fullName = '[' + name + ']';
      return String::npos != templ.find(fullName);
    }
  private:
    class TrackableValue
    {
    public:
      explicit TrackableValue(bool trackable)
        : Trackable(trackable)
        , Value(-1)
      {
      }

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
      int_t Value;
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
    FileParameters(Parameters::Accessor::Ptr params, String id)
      : Params(std::move(params))
      , Id(std::move(id))
    {
    }

    String GetFilenameTemplate() const
    {
      Parameters::StringType nameTemplate = GetProperty<Parameters::StringType>(Parameters::ZXTune::Sound::Backends::File::FILENAME.Name());
      if (nameTemplate.empty())
      {
        // Filename parameter is required
        throw Error(THIS_LINE, translate("Output filename template is not specified."));
      }
      // check if required to add extension
      const String extension = Char('.') + Id;
      const String::size_type extPos = nameTemplate.find(extension);
      if (String::npos == extPos || extPos + extension.size() != nameTemplate.size())
      {
        nameTemplate += extension;
      }
      return nameTemplate;
    }

    uint_t GetBuffersCount() const
    {
      const Parameters::IntType intParam = GetProperty<Parameters::IntType>(Parameters::ZXTune::Sound::Backends::File::BUFFERS.Name());
      return static_cast<uint_t>(intParam);
    }
  private:
    template<class T>
    T GetProperty(const std::string& name) const
    {
      T result = T();
      if (!Params->FindValue(GetBackendPropertyName(name), result))
      {
        Params->FindValue(GetComonPropertyName(name), result);
      }
      return result;
    }

    Parameters::NameType GetBackendPropertyName(const std::string& name) const
    {
      return Parameters::ZXTune::Sound::Backends::PREFIX + ToStdString(Id) + name;
    }

    Parameters::NameType GetComonPropertyName(const std::string& name) const
    {
      return Parameters::ZXTune::Sound::Backends::File::PREFIX + name;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const String Id;
  };

  String InstantiateModuleFields(const String& nameTemplate, const Parameters::Accessor& props)
  {
    Dbg("Original filename template: '%1%'", nameTemplate);
    const Parameters::FieldsSourceAdapter<Strings::KeepFieldsSource> moduleFields(props);
    const Strings::Template::Ptr templ = IO::CreateFilenameTemplate(nameTemplate);
    const String nameTemplateWithRuntimeFields = templ->Instantiate(moduleFields);
    Dbg("Fixed filename template: '%1%'", nameTemplateWithRuntimeFields);
    return nameTemplateWithRuntimeFields;
  }

  class StreamSource
  {
  public:
    StreamSource(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory)
      : Params(params)
      , FileParams(params, factory->GetId())
      , Factory(factory)
      , FilenameTemplate(InstantiateModuleFields(FileParams.GetFilenameTemplate(), *Params))
    {
    }

    Receiver::Ptr GetStream(const Module::TrackState& state) const
    {
      const String& newFilename = FilenameTemplate.Instantiate(state);
      if (Filename != newFilename)
      {
        const Binary::OutputStream::Ptr stream = IO::CreateStream(newFilename, *Params, Log::ProgressCallback::Stub());
        Filename = newFilename;
        const FileStream::Ptr result = Factory->CreateStream(stream);
        SetProperties(*result);
        if (const uint_t buffers = FileParams.GetBuffersCount())
        {
          return Async::DataReceiver<Chunk::Ptr>::Create(1, buffers, result);
        }
        else
        {
          return result;
        }
      }
      return Receiver::Ptr();
    }
  private:
    void SetProperties(FileStream& stream) const
    {
      Parameters::StringType str;
      if (Params->FindValue(Module::ATTR_TITLE, str) && !str.empty())
      {
        stream.SetTitle(str);
      }
      if (Params->FindValue(Module::ATTR_AUTHOR, str) && !str.empty())
      {
        stream.SetAuthor(str);
      }
      if (Params->FindValue(Module::ATTR_COMMENT, str) && !str.empty())
      {
        stream.SetComment(str);
      }
      else
      {
        stream.SetComment(Text::FILE_BACKEND_DEFAULT_COMMENT);
      }
      stream.FlushMetadata();
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const FileParameters FileParams;
    const FileStreamFactory::Ptr Factory;
    const TrackStateTemplate FilenameTemplate;
    mutable String Filename;
  };

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory)
      : Params(std::move(params))
      , Factory(std::move(factory))
      , Stream(Receiver::CreateStub())
    {
    }

    //BackendWorker
    void Startup() override
    {
      Source.reset(new StreamSource(Params, Factory));
    }

    void Shutdown() override
    {
      SetStream(Receiver::CreateStub());
      Source.reset();
    }

    void Pause() override
    {
    }

    void Resume() override
    {
    }

    void FrameStart(const Module::TrackState& state) override
    {
      if (const Receiver::Ptr newStream = Source->GetStream(state))
      {
        SetStream(newStream);
      }
    }

    void FrameFinish(Chunk::Ptr buffer) override
    {
      assert(Stream);
      Stream->ApplyData(std::move(buffer));
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      // Does not support volume control
      return VolumeControl::Ptr();
    }
  private:
    void SetStream(Receiver::Ptr str)
    {
      Stream->Flush();
      Stream = str;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const FileStreamFactory::Ptr Factory;
    std::unique_ptr<StreamSource> Source;
    Receiver::Ptr Stream;
  };
}//File
}//Sound

namespace Sound
{
  BackendWorker::Ptr CreateFileBackendWorker(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory)
  {
    return MakePtr<File::BackendWorker>(params, factory);
  }
}

