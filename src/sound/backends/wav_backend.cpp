/*
Abstract:
  Wave file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "backend_impl.h"
#include "enumerator.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <template_parameters.h>
#include <tools.h>
//library includes
#include <core/module_attrs.h>
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
#include <fstream>
//boost includes
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG EF5CB4C6

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Wav");

  const Char WAV_BACKEND_ID[] = {'w', 'a', 'v', 0};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  // Standard .wav header
  PACK_PRE struct WaveFormat
  {
    uint8_t Id[4];          //'RIFF'
    uint32_t Size;          //file size - 8
    uint8_t Type[4];        //'WAVE'
    uint8_t ChunkId[4];     //'fmt '
    uint32_t ChunkSize;     //16
    uint16_t Compression;   //1
    uint16_t Channels;
    uint32_t Samplerate;
    uint32_t BytesPerSec;
    uint16_t Align;
    uint16_t BitsPerSample;
    uint8_t DataId[4];      //'data'
    uint32_t DataSize;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t RIFF[] = {'R', 'I', 'F', 'F'};
  const uint8_t WAVEfmt[] = {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
  const uint8_t DATA[] = {'d', 'a', 't', 'a'};

  const Char FILE_WAVE_EXT[] = {'.', 'w', 'a', 'v', '\0'};

  class TrackProcessor
  {
  public:
    typedef std::auto_ptr<TrackProcessor> Ptr;
    virtual ~TrackProcessor() {}

    virtual void BeginFrame(const Module::TrackState& state) = 0;
    virtual void FinishFrame(const Chunk& data) = 0;
  };

  class WavBackendParameters
  {
  public:
    explicit WavBackendParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    String GetFilenameTemplate() const
    {
      Parameters::StringType nameTemplate;
      if (!Accessor.FindStringValue(Parameters::ZXTune::Sound::Backends::Wav::FILENAME, nameTemplate) ||
          nameTemplate.empty())
      {
        // Filename parameter is required
        throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_WAV_BACKEND_NO_FILENAME);
      }
      // check if required to add extension
      const String extension = FILE_WAVE_EXT;
      const String::size_type extPos = nameTemplate.find(extension);
      if (String::npos == extPos || extPos + extension.size() != nameTemplate.size())
      {
        nameTemplate += extension;
      }
      return nameTemplate;
    }

    bool CheckIfRewrite() const
    {
      Parameters::IntType intParam = 0;
      return Accessor.FindIntValue(Parameters::ZXTune::Sound::Backends::Wav::OVERWRITE, intParam) &&
        intParam != 0;
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class StateFieldsSource : public SkipFieldsSource
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
      return SkipFieldsSource::GetFieldValue(fieldName);
    }
  private:
    const Module::TrackState& State;
  };

  class FileWriter : public TrackProcessor
  {
  public:
    FileWriter(uint_t soundFreq, const String& fileNameTemplate, bool doRewrite)
      : DoRewrite(doRewrite)
    {
      //init struct
      std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      std::memcpy(Format.Type, WAVEfmt, sizeof(WAVEfmt));
      Format.ChunkSize = fromLE<uint32_t>(16);
      Format.Compression = fromLE<uint16_t>(1);//PCM
      Format.Channels = fromLE<uint16_t>(OUTPUT_CHANNELS);
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
      Format.Samplerate = fromLE(static_cast<uint32_t>(soundFreq));
      Format.BytesPerSec = fromLE(static_cast<uint32_t>(soundFreq * sizeof(MultiSample)));
      Format.Align = fromLE<uint16_t>(sizeof(MultiSample));
      Format.BitsPerSample = fromLE<uint16_t>(8 * sizeof(Sample));
      //init generator
      NameGenerator = StringTemplate::Create(fileNameTemplate);
      //check if generator is required
      if (NameGenerator->Instantiate(SkipFieldsSource()) == fileNameTemplate)
      {
        StartFile(fileNameTemplate);
        NameGenerator.reset();
      }
    }

    virtual ~FileWriter()
    {
      StopFile();
    }

    void BeginFrame(const Module::TrackState& state)
    {
      if (NameGenerator.get())
      {
        const StateFieldsSource fields(state);
        StartFile(NameGenerator->Instantiate(fields));
      }
    }

    void FinishFrame(const Chunk& data)
    {
      const std::size_t sizeInBytes = data.size() * sizeof(data.front());
      File->write(safe_ptr_cast<const char*>(&data[0]), static_cast<std::streamsize>(sizeInBytes));
      Format.Size += static_cast<uint32_t>(sizeInBytes);
      Format.DataSize += static_cast<uint32_t>(sizeInBytes);
    }
  private:
    void StartFile(const String& filename)
    {
      if (filename != Filename)
      {
        StopFile();
        Log::Debug(THIS_MODULE, "Opening file '%1%'", filename);
        File = IO::CreateFile(filename, DoRewrite);
        //swap on final
        Format.Size = sizeof(Format) - 8;
        Format.DataSize = 0;
        //skip header
        File->seekp(sizeof(Format));
        Filename = filename;
      }
    }

    void StopFile()
    {
      if (File.get())
      {
        Log::Debug(THIS_MODULE, "Closing file '%1%'", Filename);
        // write header
        File->seekp(0);
        Format.Size = fromLE(Format.Size);
        Format.DataSize = fromLE(Format.DataSize);
        File->write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
        File.reset();
      }
    }
  private:
    const bool DoRewrite;
    StringTemplate::Ptr NameGenerator;
    String Filename;
    std::auto_ptr<std::ofstream> File;
    WaveFormat Format;
  };

  class ModuleFieldsSource : public Parameters::FieldsSourceAdapter<KeepFieldsSource<'[', ']'> >
  {
  public:
    typedef Parameters::FieldsSourceAdapter<KeepFieldsSource<'[', ']'> > Parent;
    explicit ModuleFieldsSource(const Parameters::Accessor& params)
      : Parent(params)
    {
    }

    String GetFieldValue(const String& fieldName) const
    {
      return IO::MakePathFromString(Parent::GetFieldValue(fieldName), '_');
    }
  };

  class ComplexTrackProcessor : public TrackProcessor
  {
  public:
    ComplexTrackProcessor(const RenderParameters& soundParams, const WavBackendParameters& backendParameters, const Parameters::Accessor& props)
    {
      //acquire name template
      const String nameTemplate = backendParameters.GetFilenameTemplate();
      Log::Debug(THIS_MODULE, "Original filename template: '%1%'", nameTemplate);
      const ModuleFieldsSource moduleFields(props);
      const String fileName = InstantiateTemplate(nameTemplate, moduleFields);
      Log::Debug(THIS_MODULE, "Fixed filename template: '%1%'", fileName);
      const bool doRewrite = backendParameters.CheckIfRewrite();
      Writer.reset(new FileWriter(soundParams.SoundFreq(), fileName, doRewrite));
    }

    void BeginFrame(const Module::TrackState& state)
    {
      Writer->BeginFrame(state);
    }

    void FinishFrame(const Chunk& data)
    {
      Writer->FinishFrame(data);
    }
  private:
    TrackProcessor::Ptr Writer;
  };

  class WAVBackendWorker : public BackendWorker
                         , private boost::noncopyable
  {
  public:
    explicit WAVBackendWorker(Parameters::Accessor::Ptr params)
      : BackendParams(params)
      , RenderingParameters(RenderParameters::Create(BackendParams))
    {
    }

    virtual ~WAVBackendWorker()
    {
      assert(!Processor.get() || !"FileBackend::Stop should be called before exit");
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
      assert(!Processor.get());

      const Parameters::Accessor::Ptr props = module.GetModuleProperties();
      const WavBackendParameters backendParameters(*BackendParams);
      Processor.reset(new ComplexTrackProcessor(*RenderingParameters, backendParameters, *props));
    }

    virtual void OnShutdown()
    {
      Processor.reset();
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnFrame(const Module::TrackState& state)
    {
      Processor->BeginFrame(state);
    }

    virtual void OnBufferReady(Chunk& buffer)
    {
#ifdef BOOST_BIG_ENDIAN
      // in case of big endian, required to swap values
      Buffer.resize(buffer.size());
      std::transform(buffer.front().begin(), buffer.back().end(), Buffer.front().begin(), &swapBytes<Sample>);
      Processor->FinishFrame(Buffer);
#else
      Processor->FinishFrame(buffer);
#endif
    }
  private:
    const Parameters::Accessor::Ptr BackendParams;
    const RenderParameters::Ptr RenderingParameters;
    TrackProcessor::Ptr Processor;
#ifdef BOOST_BIG_ENDIAN
    Chunk Buffer;
#endif
  };

  class WAVBackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return WAV_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::WAV_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new WAVBackendWorker(allParams));
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE,
          Text::SOUND_ERROR_BACKEND_FAILED, Id()).AddSuberror(e);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
      }
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWAVBackend(BackendsEnumerator& enumerator)
    {
      const BackendCreator::Ptr creator(new WAVBackendCreator());
      enumerator.RegisterCreator(creator);
    }
  }
}
