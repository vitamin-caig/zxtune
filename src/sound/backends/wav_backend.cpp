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
#include <formatter.h>
#include <logging.h>
#include <template.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
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

  const std::string THIS_MODULE("WavBackend");

  const Char BACKEND_ID[] = {'w', 'a', 'v', 0};
  const String BACKEND_VERSION(FromStdString("$Rev$"));

  // backend description
  const BackendInformation BACKEND_INFO =
  {
    BACKEND_ID,
    Text::WAV_BACKEND_DESCRIPTION,
    BACKEND_VERSION,
  };

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

  class TrackLogger
  {
  public:
    typedef std::auto_ptr<TrackLogger> Ptr;
    virtual ~TrackLogger() {}

    virtual void LogFrame(uint_t frame, const Module::Tracking& track) = 0;
  };

  const uint_t MIN_CUESHEET_PERIOD = 4;
  const uint_t MAX_CUESHEET_PERIOD = 48;

  class CuesheetLogger : public TrackLogger
  {
  public:
    explicit CuesheetLogger(const std::string& wavName, const Parameters::Map& params)
      : File(IO::CreateFile(wavName + FILE_CUE_EXT, true))
      , FrameDuration(Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT)
      , SplitPeriod(0)
    {
      String path;
      *File << (Formatter(Text::CUESHEET_BEGIN) % IO::ExtractLastPathComponent(wavName, path)).str();
      Parameters::FindByName(params, Parameters::ZXTune::Sound::FRAMEDURATION, FrameDuration);
      if (Parameters::FindByName(params, Parameters::ZXTune::Sound::Backends::Wav::CUESHEET_PERIOD, SplitPeriod) &&
          !in_range<Parameters::IntType>(SplitPeriod, MIN_CUESHEET_PERIOD, MAX_CUESHEET_PERIOD))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::CUESHEET_ERROR_INVALID_PERIOD, SplitPeriod, MIN_CUESHEET_PERIOD, MAX_CUESHEET_PERIOD);
      }
      Track.Position = -1;
    }

    void LogFrame(uint_t frame, const Module::Tracking& track)
    {
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
    std::auto_ptr<std::ofstream> File;
    Module::Tracking Track;
    Parameters::IntType FrameDuration;
    Parameters::IntType SplitPeriod;
  };

  class WAVBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    WAVBackend() : File()
    {
      std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      std::memcpy(Format.Type, WAVEfmt, sizeof(WAVEfmt));
      Format.ChunkSize = fromLE<uint32_t>(16);
      Format.Compression = fromLE<uint16_t>(1);//PCM
      Format.Channels = fromLE<uint16_t>(OUTPUT_CHANNELS);
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
    }

    virtual ~WAVBackend()
    {
      assert(!File.get() || !"FileBackend::Stop should be called before exit");
    }

    virtual void GetInformation(BackendInformation& info) const
    {
      info = BACKEND_INFO;
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      // Does not support volume control
      return VolumeControl::Ptr();
    }

    virtual void OnStartup()
    {
      assert(!File.get());
    
      String nameTemplate;
      if (!Parameters::FindByName(CommonParameters, Parameters::ZXTune::Sound::Backends::Wav::FILENAME, nameTemplate))
      {
        // Filename parameter is required
        throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_WAV_BACKEND_NO_FILENAME);
      }

      //if playback now
      if (Player)
      {
        // check if required to add extension
        const String extension = FILE_WAVE_EXT;
        const String::size_type extPos = nameTemplate.find(extension);
        if (String::npos == extPos || extPos + extension.size() != nameTemplate.size())
        {
          nameTemplate += extension;
        }

        Module::Information info;
        Player->GetModule().GetModuleInformation(info);
        StringMap strProps;
        //quote all properties for safe using as filename
        {
          StringMap tmpProps;
          Parameters::ConvertMap(info.Properties, tmpProps);
          std::transform(tmpProps.begin(), tmpProps.end(), std::inserter(strProps, strProps.end()),
            boost::bind(&std::make_pair<String, String>,
              boost::bind<String>(&StringMap::value_type::first, _1),
              boost::bind<String>(&IO::MakePathFromString,
                boost::bind<String>(&StringMap::value_type::second, _1), '_')));
        }
        const String& fileName = InstantiateTemplate(nameTemplate, strProps, SKIP_NONEXISTING);
        Log::Debug(THIS_MODULE, "Opening file '%1%'", fileName);
        //(re)create file
        {
          Parameters::IntType intParam = 0;
          const bool doRewrite = Parameters::FindByName(CommonParameters, Parameters::ZXTune::Sound::Backends::Wav::OVERWRITE, intParam) &&
            intParam != 0;
          std::auto_ptr<std::ofstream> tmpFile = IO::CreateFile(fileName, doRewrite);
          //prepare cuesheet if required
          if (Parameters::FindByName(CommonParameters, Parameters::ZXTune::Sound::Backends::Wav::CUESHEET, intParam) &&
            intParam != 0)
          {
            Logger.reset(new CuesheetLogger(fileName, CommonParameters));
            UpdateLogger();
          }
          File = tmpFile;
        }

        File->seekp(sizeof(Format));

        Format.Samplerate = fromLE(static_cast<uint32_t>(RenderingParameters.SoundFreq));
        Format.BytesPerSec = fromLE(static_cast<uint32_t>(RenderingParameters.SoundFreq * sizeof(MultiSample)));
        Format.Align = fromLE<uint16_t>(sizeof(MultiSample));
        Format.BitsPerSample = fromLE<uint16_t>(8 * sizeof(Sample));
        //swap on final
        Format.Size = sizeof(Format) - 8;
        Format.DataSize = 0;
      }
    }

    virtual void OnShutdown()
    {
      if (File.get())
      {
        // write header
        File->seekp(0);
        Format.Size = fromLE(Format.Size);
        Format.DataSize = fromLE(Format.DataSize);
        File->write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
        File.reset();
        Logger.reset();
      }
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnParametersChanged(const Parameters::Map& updates)
    {
      if (File.get() &&
          (Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY) ||
           Parameters::FindByName<Parameters::StringType>(updates, Parameters::ZXTune::Sound::Backends::Wav::FILENAME)))
      {
        // changing filename and frequency 'on fly' is not supported
        throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, Text::SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      const std::size_t sizeInBytes = buffer.size() * sizeof(buffer.front());
#ifdef BOOST_BIG_ENDIAN
      // in case of big endian, required to swap values
      Buffer.resize(buffer.size());
      std::transform(buffer.front().begin(), buffer.back().end(), Buffer.front().begin(), &swapBytes<Sample>);
      File->write(safe_ptr_cast<const char*>(&Buffer[0]), static_cast<std::streamsize>(sizeInBytes));
#else
      File->write(safe_ptr_cast<const char*>(&buffer[0]), static_cast<std::streamsize>(sizeInBytes));
#endif
      Format.Size += static_cast<uint32_t>(sizeInBytes);
      Format.DataSize += static_cast<uint32_t>(sizeInBytes);
      //write track log
      UpdateLogger();
    }
  private:
    void UpdateLogger() const
    {
      if (Logger.get())
      {
        uint_t time = 0;
        Module::Tracking track;
        Module::Analyze::ChannelsState analyze;
        ThrowIfError(Player->GetPlaybackState(time, track, analyze));
        Logger->LogFrame(time, track);
      }
    }
  private:
    std::auto_ptr<std::ofstream> File;
    WaveFormat Format;
#ifdef BOOST_BIG_ENDIAN
    std::vector<MultiSample> Buffer;
#endif
    TrackLogger::Ptr Logger;
  };
  
  Backend::Ptr WAVBackendCreator(const Parameters::Map& params)
  {
    return Backend::Ptr(new SafeBackendWrapper<WAVBackend>(params));
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWAVBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, &WAVBackendCreator);
    }
  }
}
