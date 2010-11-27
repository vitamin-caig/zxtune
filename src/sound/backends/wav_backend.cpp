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
#include "backend_wrapper.h"
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
//std includes
#include <algorithm>
#include <fstream>
//boost includes
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG EF5CB4C6

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("WavBackend");

  const Char WAV_BACKEND_ID[] = {'w', 'a', 'v', 0};
  const String WAV_BACKEND_VERSION(FromStdString("$Rev$"));

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
  const Char FILE_CUE_EXT[] = {'.', 'c', 'u', 'e', '\0'};

  class TrackProcessor
  {
  public:
    typedef std::auto_ptr<TrackProcessor> Ptr;
    virtual ~TrackProcessor() {}

    virtual void BeginFrame(const Module::TrackState& state) = 0;
    virtual void FinishFrame(const std::vector<MultiSample>& data) = 0;
  };

  const uint_t MIN_CUESHEET_PERIOD = 4;
  const uint_t MAX_CUESHEET_PERIOD = 48;

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

    bool HasCuesheet() const
    {
      Parameters::IntType intParam = 0;
      return Accessor.FindIntValue(Parameters::ZXTune::Sound::Backends::Wav::CUESHEET, intParam) &&
        intParam != 0;
    }

    uint_t GetSplitPeriod() const
    {
      Parameters::IntType period = 0;

      if (Accessor.FindIntValue(Parameters::ZXTune::Sound::Backends::Wav::CUESHEET_PERIOD, period) &&
          !in_range<Parameters::IntType>(period, MIN_CUESHEET_PERIOD, MAX_CUESHEET_PERIOD))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::CUESHEET_ERROR_INVALID_PERIOD, period, MIN_CUESHEET_PERIOD, MAX_CUESHEET_PERIOD);
      }
      return static_cast<uint_t>(period);
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  class CuesheetLogger : public TrackProcessor
  {
  public:
    explicit CuesheetLogger(const String& wavName, const RenderParameters& sndParams, const WavBackendParameters& wavParams)
      : File(IO::CreateFile(wavName + FILE_CUE_EXT, true))
      , FrameDuration(sndParams.FrameDurationMicrosec())
      , SplitPeriod(wavParams.GetSplitPeriod())
      , PrevPosition(~0)
      , PrevLine(0)
    {
      String path;
      *File << (Formatter(Text::CUESHEET_BEGIN) % IO::ExtractLastPathComponent(wavName, path)).str();
    }

    void BeginFrame(const Module::TrackState& state)
    {
      const uint_t curPosition = state.Position();
      const uint_t curLine = state.Line();
      const bool positionChanged = PrevPosition != curPosition;
      const bool splitOcurred =
           SplitPeriod != 0 && //period specified
           0 == state.Quirk() && //new line
           0 == curLine % SplitPeriod && //line number is multiple of period
           (positionChanged || curLine > PrevLine); //to prevent reseting line to 0 after an end

      if (positionChanged)
      {
        *File << (Formatter(Text::CUESHEET_TRACK_ITEM)
          % (curPosition + 1)
          % state.Pattern()).str();
      }
      if (positionChanged || splitOcurred)
      {
        const uint_t curFrame = state.Frame();
        const uint_t CUEFRAMES_PER_SECOND = 75;
        const uint_t CUEFRAMES_PER_MINUTE = 60 * CUEFRAMES_PER_SECOND;
        const uint_t cueTotalFrames = static_cast<uint_t>(uint64_t(curFrame) * FrameDuration * CUEFRAMES_PER_SECOND / 1000000);
        const uint_t cueMinutes = cueTotalFrames / CUEFRAMES_PER_MINUTE;
        const uint_t cueSeconds = (cueTotalFrames - cueMinutes * CUEFRAMES_PER_MINUTE) / CUEFRAMES_PER_SECOND;
        const uint_t cueFrames = cueTotalFrames % CUEFRAMES_PER_SECOND;
        *File << (Formatter(Text::CUESHEET_TRACK_INDEX)
          % (SplitPeriod ? curLine / SplitPeriod + 1 : 1)
          % cueMinutes % cueSeconds % cueFrames).str();
        PrevPosition = curPosition;
        PrevLine = curLine;
      }
    }

    void FinishFrame(const std::vector<MultiSample>&)
    {
    }
  private:
    const std::auto_ptr<std::ofstream> File;
    const uint_t FrameDuration;
    const uint_t SplitPeriod;
    uint_t PrevPosition;
    uint_t PrevLine;
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

    void FinishFrame(const std::vector<MultiSample>& data)
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
    ComplexTrackProcessor(const RenderParameters& soundParams, const WavBackendParameters& backendParameters, const Module::Information& info)
    {
      //acquire name template
      const String nameTemplate = backendParameters.GetFilenameTemplate();
      Log::Debug(THIS_MODULE, "Original filename template: '%1%'", nameTemplate);
      const ModuleFieldsSource moduleFields(*info.Properties());
      const String fileName = InstantiateTemplate(nameTemplate, moduleFields);
      Log::Debug(THIS_MODULE, "Fixed filename template: '%1%'", fileName);
      const bool doRewrite = backendParameters.CheckIfRewrite();
      Writer.reset(new FileWriter(soundParams.SoundFreq(), fileName, doRewrite));
      //prepare cuesheet if required
      if (backendParameters.HasCuesheet())
      {
        Logger.reset(new CuesheetLogger(fileName, soundParams, backendParameters));
      }
    }

    void BeginFrame(const Module::TrackState& state)
    {
      Writer->BeginFrame(state);
      if (Logger.get())
      {
        Logger->BeginFrame(state);
      }
    }

    void FinishFrame(const std::vector<MultiSample>& data)
    {
      Writer->FinishFrame(data);
      if (Logger.get())
      {
        Logger->FinishFrame(data);
      }
    }
  private:
    TrackProcessor::Ptr Writer;
    TrackProcessor::Ptr Logger;
  };

  class WAVBackend : public BackendImpl
                   , private boost::noncopyable
  {
  public:
    explicit WAVBackend(Parameters::Accessor::Ptr soundParams)
      : BackendImpl(soundParams)
    {
    }
    virtual ~WAVBackend()
    {
      assert(!Processor.get() || !"FileBackend::Stop should be called before exit");
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      // Does not support volume control
      return VolumeControl::Ptr();
    }

    virtual void OnStartup()
    {
      assert(!Processor.get());

      //if playback now
      if (Player)
      {
        const Module::Information::Ptr info = Player->GetInformation();
        const WavBackendParameters backendParameters(*SoundParameters);
        Processor.reset(new ComplexTrackProcessor(*RenderingParameters, backendParameters, *info));
        State = Player->GetTrackState();
      }
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

    virtual bool OnRenderFrame()
    {
      Processor->BeginFrame(*State);
      return BackendImpl::OnRenderFrame();
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
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

    virtual void OnParametersChanged(const Parameters::Accessor& /*params*/)
    {
      if (Processor.get())
      {
        // changing any of the properties 'on fly' is not supported
        throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }
  private:
    TrackProcessor::Ptr Processor;
    Module::TrackState::Ptr State;
#ifdef BOOST_BIG_ENDIAN
    std::vector<MultiSample> Buffer;
#endif
  };

  class WAVBackendCreator : public BackendCreator
                          , public boost::enable_shared_from_this<WAVBackendCreator>
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

    virtual String Version() const
    {
      return WAV_BACKEND_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error CreateBackend(Parameters::Accessor::Ptr params, Backend::Ptr& result) const
    {
      const BackendInformation::Ptr info = shared_from_this();
      return SafeBackendWrapper<WAVBackend>::Create(info, params, result, THIS_LINE);
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
