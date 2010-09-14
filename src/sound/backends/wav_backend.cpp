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

    virtual void OnFrame(const Module::State& state, const std::vector<MultiSample>& data) = 0;
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

    uint_t GetFrameDuration() const
    {
      Parameters::IntType res = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
      Accessor.FindIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, res);
      return static_cast<uint_t>(res);
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
    explicit CuesheetLogger(const String& wavName, const WavBackendParameters& params)
      : File(IO::CreateFile(wavName + FILE_CUE_EXT, true))
      , FrameDuration(params.GetFrameDuration())
      , SplitPeriod(params.GetSplitPeriod())
    {
      String path;
      *File << (Formatter(Text::CUESHEET_BEGIN) % IO::ExtractLastPathComponent(wavName, path)).str();
      Track.Position = -1;
    }

    void OnFrame(const Module::State& state, const std::vector<MultiSample>&)
    {
      const uint_t frame = state.Frame;
      const Module::Tracking& track = state.Track;
      const bool positionChanged = track.Position != Track.Position;
      const bool splitOcurred =
           SplitPeriod != 0 && //period specified
           0 == track.Frame && //new line
           0 == track.Line % SplitPeriod && //line number is multiple of period
           (positionChanged || track.Line > Track.Line); //to prevent reseting line to 0 after an end

      if (positionChanged)
      {
        *File << (Formatter(Text::CUESHEET_TRACK_ITEM)
          % (track.Position + 1)
          % track.Pattern).str();
      }
      if (positionChanged || splitOcurred)
      {
        const uint_t CUEFRAMES_PER_SECOND = 75;
        const uint_t CUEFRAMES_PER_MINUTE = 60 * CUEFRAMES_PER_SECOND;
        const uint_t cueTotalFrames = static_cast<uint_t>(uint64_t(frame) * FrameDuration * CUEFRAMES_PER_SECOND / 1000000);
        const uint_t cueMinutes = cueTotalFrames / CUEFRAMES_PER_MINUTE;
        const uint_t cueSeconds = (cueTotalFrames - cueMinutes * CUEFRAMES_PER_MINUTE) / CUEFRAMES_PER_SECOND;
        const uint_t cueFrames = cueTotalFrames % CUEFRAMES_PER_SECOND;
        *File << (Formatter(Text::CUESHEET_TRACK_INDEX)
          % (SplitPeriod ? track.Line / SplitPeriod + 1 : 1)
          % cueMinutes % cueSeconds % cueFrames).str();
        Track = track;
      }
    }
  private:
    const std::auto_ptr<std::ofstream> File;
    Module::Tracking Track;
    const uint_t FrameDuration;
    const uint_t SplitPeriod;
  };

  class StateFieldsSource : public SkipFieldsSource
  {
  public:
    explicit StateFieldsSource(const Module::State& state)
      : State(state)
    {
    }
    
    String GetFieldValue(const String& fieldName) const
    {
      if (fieldName == Module::ATTR_CURRENT_POSITION)
      {
        return Parameters::ConvertToString(Parameters::ValueType(State.Track.Position));
      }
      else if (fieldName == Module::ATTR_CURRENT_PATTERN)
      {
        return Parameters::ConvertToString(Parameters::ValueType(State.Track.Pattern));
      }
      else if (fieldName == Module::ATTR_CURRENT_LINE)
      {
        return Parameters::ConvertToString(Parameters::ValueType(State.Track.Line));
      }
      return SkipFieldsSource::GetFieldValue(fieldName);
    }
  private:
    const Module::State& State;
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

    void OnFrame(const Module::State& state, const std::vector<MultiSample>& data)
    {
      if (NameGenerator.get())
      {
        const StateFieldsSource fields(state);
        StartFile(NameGenerator->Instantiate(fields));
      }
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
    ComplexTrackProcessor(const Parameters::Accessor& commonParams, const RenderParameters& soundParams, const Module::Information& info)
    {
      const WavBackendParameters backendParameters(commonParams);
      //acquire name template
      const String nameTemplate = backendParameters.GetFilenameTemplate();
      const ModuleFieldsSource moduleFields(*info.Properties());
      const String fileName = InstantiateTemplate(nameTemplate, moduleFields);
      const bool doRewrite = backendParameters.CheckIfRewrite();
      Writer.reset(new FileWriter(soundParams.SoundFreq, fileName, doRewrite));
      //prepare cuesheet if required
      if (backendParameters.HasCuesheet())
      {
        Logger.reset(new CuesheetLogger(fileName, backendParameters));
      }
    }

    void OnFrame(const Module::State& state, const std::vector<MultiSample>& data)
    {
      Writer->OnFrame(state, data);
      if (Logger.get())
      {
        Logger->OnFrame(state, data);
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
      if (Player && Holder)
      {
        const Module::Information::Ptr info = Holder->GetModuleInformation();
        Processor.reset(new ComplexTrackProcessor(*CommonParameters, RenderingParameters, *info));
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

    virtual void OnParametersChanged(const Parameters::Accessor& /*params*/)
    {
      if (Processor.get())
      {
        // changing any of the properties 'on fly' is not supported
        throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      //state
      Module::State state;
      Module::Analyze::ChannelsState analyze;
      ThrowIfError(Player->GetPlaybackState(state, analyze));
      //form result
#ifdef BOOST_BIG_ENDIAN
      // in case of big endian, required to swap values
      Buffer.resize(buffer.size());
      std::transform(buffer.front().begin(), buffer.back().end(), Buffer.front().begin(), &swapBytes<Sample>);
      Processor->OnFrame(state, Buffer);
#else
      Processor->OnFrame(state, buffer);
#endif
    }
  private:
    TrackProcessor::Ptr Processor;
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

    virtual Error CreateBackend(const Parameters::Accessor& params, Backend::Ptr& result) const
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
