/*
Abstract:
  File-based backends support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "file_backend.h"
//common includes
#include <debug_log.h>
#include <template_parameters.h>
//library includes
#include <async/data_receiver.h>
#include <core/module_attrs.h>
#include <io/providers_parameters.h>
#include <io/providers/file_provider.h>
#include <l10n/api.h>
#include <sound/backends_parameters.h>
#include <strings/template.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/backends.h>

#define FILE_TAG B4CB6B0C

namespace
{
  const Debug::Stream Dbg("Sound::Backend::FileBase");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");
}

namespace
{
  using namespace ZXTune;
  using namespace Sound;

  const Char FILENAME_PARAM[] = {'f', 'i', 'l', 'e', 'n', 'a', 'm', 'e', '\0'};
  const Char OVERWRITE_PARAM[] = {'o', 'v', 'e', 'r', 'w', 'r', 'i', 't', 'e', '\0'};

  const Char COMMON_FILE_BACKEND_ID[] = {'f', 'i', 'l', 'e', '\0'};

  class StateFieldsSource : public Strings::SkipFieldsSource
  {
  public:
    explicit StateFieldsSource(const Module::TrackState& state)
      : State(state)
    {
    }

    String GetFieldValue(const String& fieldName) const
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

  class FileParameters : public ZXTune::IO::FileCreatingParameters
  {
  public:
    FileParameters(Parameters::Accessor::Ptr params, const String& id)
      : Params(params)
      , Id(id)
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

    virtual bool Overwrite() const
    {
      Parameters::IntType intParam = Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT;
      Params->FindValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, intParam);
      return intParam != 0;
    }

    virtual bool CreateDirectories() const
    {
      Parameters::IntType intParam = Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT;
      Params->FindValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, intParam);
      return intParam != 0;
    }

    virtual bool SanitizeNames() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::SANITIZE_NAMES_DEFAULT;
      Params->FindValue(Parameters::ZXTune::IO::Providers::File::SANITIZE_NAMES, intVal);
      return intVal != 0;
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
    const Parameters::FieldsSourceAdapter<Strings::KeepFieldsSource<'[', ']'> > moduleFields(props);
    const String nameTemplateWithRuntimeFields = Strings::Template::Instantiate(nameTemplate, moduleFields);
    Dbg("Fixed filename template: '%1%'", nameTemplateWithRuntimeFields);
    return nameTemplateWithRuntimeFields;
  }

  class StreamSource
  {
  public:
    StreamSource(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory, Parameters::Accessor::Ptr properties)
      : Params(params, factory->GetId())
      , Factory(factory)
      , Properties(properties)
      , FilenameTemplate(InstantiateModuleFields(Params.GetFilenameTemplate(), *properties))
    {
    }

    ChunkStream::Ptr GetStream(const Module::TrackState& state) const
    {
      const String& newFilename = FilenameTemplate.Instantiate(state);
      if (Filename != newFilename)
      {
        //TODO: use most common API to create stream
        const Binary::OutputStream::Ptr stream = ZXTune::IO::CreateLocalFile(newFilename, Params);
        Filename = newFilename;
        const FileStream::Ptr result = Factory->CreateStream(stream);
        SetProperties(*result);
        if (const uint_t buffers = Params.GetBuffersCount())
        {
          return Async::DataReceiver<ChunkPtr>::Create(1, buffers, result);
        }
        else
        {
          return result;
        }
      }
      return ChunkStream::Ptr();
    }
  private:
    void SetProperties(FileStream& stream) const
    {
      Parameters::StringType str;
      if (Properties->FindValue(Module::ATTR_TITLE, str) && !str.empty())
      {
        stream.SetTitle(str);
      }
      if (Properties->FindValue(Module::ATTR_AUTHOR, str) && !str.empty())
      {
        stream.SetAuthor(str);
      }
      if (Properties->FindValue(Module::ATTR_COMMENT, str) && !str.empty())
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
    const FileParameters Params;
    const FileStreamFactory::Ptr Factory;
    const Parameters::Accessor::Ptr Properties;
    const TrackStateTemplate FilenameTemplate;
    mutable String Filename;
  };

  class FileBackendWorker : public BackendWorker
  {
  public:
    FileBackendWorker(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory)
      : Params(params)
      , Factory(factory)
      , Stream(ChunkStream::CreateStub())
    {
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      // Does not support volume control
      return VolumeControl::Ptr();
    }

    virtual void Test()
    {
      //TODO: check for write permissions
    }

    virtual void OnStartup(const Module::Holder& module)
    {
      const Parameters::Accessor::Ptr props = module.GetModuleProperties();
      Source.reset(new StreamSource(Params, Factory, props));
    }

    virtual void OnShutdown()
    {
      SetStream(ChunkStream::CreateStub());
      Source.reset();
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnFrame(const Module::TrackState& state)
    {
      if (ChunkStream::Ptr newStream = Source->GetStream(state))
      {
        SetStream(newStream);
      }
    }

    virtual void OnBufferReady(Chunk& buffer)
    {
      assert(Stream);
      const ChunkPtr chunk = boost::make_shared<Chunk>();
      chunk->swap(buffer);
      Stream->ApplyData(chunk);
    }
  private:
    void SetStream(ChunkStream::Ptr str)
    {
      Stream->Flush();
      Stream = str;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const FileStreamFactory::Ptr Factory;
    std::auto_ptr<StreamSource> Source;
    ChunkStream::Ptr Stream;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    BackendWorker::Ptr CreateFileBackendWorker(Parameters::Accessor::Ptr params, FileStreamFactory::Ptr factory)
    {
      return boost::make_shared<FileBackendWorker>(params, factory);
    }
  }
}
