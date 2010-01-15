/*
Abstract:
  Wave file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"

#include <byteorder.h>
#include <tools.h>
#include <io/error_codes.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>

#include <boost/noncopyable.hpp>

#include <algorithm>
#include <fstream>

#include <text/backends.h>
#include <text/sound.h>

#define FILE_TAG EF5CB4C6

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Char BACKEND_ID[] = {'w', 'a', 'v', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));

  static const BackendInfo BACKEND_INFO =
  {
    BACKEND_ID,
    TEXT_WAV_BACKEND_DESCRIPTION,
    BACKEND_VERSION,
  };

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
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

  //const String::value_type STDIN_NAME[] = {'-', '\0'};

/*
  //custom tags values
  const String::value_type CHIP_YM[] = {'Y', 'M', 0};
  const String::value_type DECIBELL[] = {'d', 'B', 0};

  void Annotate(const Module::Information& modInfo, Backend::Parameters& playInfo, const String& filename)
  {
    std::basic_ofstream<String::value_type> infoFile(filename.c_str());
    if (!infoFile)
    {
      throw MakeFormattedError(ERROR_DETAIL, 1, TEXT_ERROR_OPEN_FILE, filename);
    }
    Formatter fmt(FILE_ANNOTATION_FORMAT);
    //store module information
    for (StringMap::const_iterator it = modInfo.Properties.begin(), lim = modInfo.Properties.end(); it != lim; ++it)
    {
      if (it->first == Module::ATTR_FILENAME)
      {
        String dir, filename, subname;
        IO::SplitFSName(it->second, dir, filename, subname);
        if (subname.empty())//no subname
        {
          infoFile << fmt % Module::ATTR_FILENAME % filename;
        }
        else
        {
          infoFile << fmt % Module::ATTR_ALBUM % filename;
          infoFile << fmt % Module::ATTR_FILENAME % subname;
        }
      }
      else if (it->first != Module::ATTR_WARNINGS)
      {
        infoFile << fmt % it->first % it->second;
      }
    }
    const std::size_t realFramesTotal = modInfo.Statistic.Frame * playInfo.SoundParameters.FrameDurationMicrosec / 20000;
    const std::size_t secondsTotal = realFramesTotal / 50;
    const std::size_t minutesTotal = secondsTotal / 60;
    const std::size_t hoursTotal = secondsTotal / 3600;
    infoFile << fmt % Module::ATTR_DURATION % boost::io::group(
      hoursTotal, ':',
      minutesTotal % 60, ':',
      secondsTotal % 60, '.', realFramesTotal % 50);
    infoFile << fmt % Module::ATTR_CHANNELS % modInfo.Statistic.Channels;
    //store playback information
    infoFile << fmt % Module::ATTR_CLOCKFREQ % playInfo.SoundParameters.ClockFreq;
    infoFile << fmt % Module::ATTR_FPS % boost::io::group(std::setprecision(3), 1e6f / playInfo.SoundParameters.FrameDurationMicrosec);
    if (playInfo.SoundParameters.Flags & PSG_TYPE_YM)
    {
      infoFile << fmt % Module::ATTR_CHIP % CHIP_YM;
    }
    if (playInfo.Mixer->Preamp != FIXED_POINT_PRECISION)
    {
      infoFile << fmt % Module::ATTR_GAIN %
        boost::io::group(std::setprecision(3), 20.0 * log(double(FIXED_POINT_PRECISION) / playInfo.Mixer->Preamp), DECIBELL);
    }
    if (playInfo.FIROrder)
    {
      infoFile << fmt % Module::ATTR_FILTER % boost::io::group(playInfo.LowCutoff, '-', playInfo.HighCutoff, '@', playInfo.FIROrder);
    }
  }

  void Descriptor(Backend::Info& info);
*/
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
      assert(!File.is_open() || !"FileBackend::Stop should be called before exit");
    }

    virtual void GetInfo(BackendInfo& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& /*volume*/) const
    {
      return Error(THIS_LINE, BACKEND_UNSUPPORTED_FUNC, TEXT_SOUND_ERROR_BACKEND_UNSUPPORTED_VOLUME);
    }

    virtual Error SetVolume(const MultiGain& /*volume*/)
    {
      return Error(THIS_LINE, BACKEND_UNSUPPORTED_FUNC, TEXT_SOUND_ERROR_BACKEND_UNSUPPORTED_VOLUME);
    }

    virtual void OnStartup()
    {
      assert(!File.is_open());
    
      String nameTemplate;
      if (const Parameters::StringType* wavname =
        Parameters::FindByName<Parameters::StringType>(CommonParameters, Parameters::ZXTune::Sound::Backends::Wav::FILENAME))
      {
        nameTemplate = *wavname;
      }
      else
      {
        nameTemplate = FILE_DEFAULT_NAME;
      }
      const String filename(nameTemplate + FILE_WAVE_EXT);//TODO
      File.open(filename.c_str(), std::ios::binary);
      if (!File.is_open())
      {
        throw Error(THIS_LINE, IO::NOT_OPENED);//TODO
      }
      File.seekp(sizeof(Format));

      Format.Samplerate = fromLE<uint32_t>(RenderingParameters.SoundFreq);
      Format.BytesPerSec = fromLE<uint32_t>(RenderingParameters.SoundFreq * sizeof(MultiSample));
      Format.Align = fromLE<uint16_t>(sizeof(MultiSample));
      Format.BitsPerSample = fromLE<uint16_t>(8 * sizeof(Sample));
      //swap on final
      Format.Size = sizeof(Format) - 8;
      Format.DataSize = 0;
    }

    virtual void OnShutdown()
    {
      if (File.is_open())
      {
        File.seekp(0);
        Format.Size = fromLE(Format.Size);
        Format.DataSize = fromLE(Format.DataSize);
        File.write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
        File.close();
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
      if (File.is_open() &&
          (Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY) ||
           Parameters::FindByName<Parameters::StringType>(updates, Parameters::ZXTune::Sound::Backends::Wav::FILENAME)))
      {
        throw Error(THIS_LINE, BACKEND_INVALID_PARAMETER, TEXT_SOUND_ERROR_BACKEND_INVALID_STATE);
      }
    }


    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      const std::size_t sizeInBytes = buffer.size() * sizeof(buffer.front());
#ifdef BOOST_BIG_ENDIAN
      Buffer.resize(buffer.size());
      std::transform(buffer.begin()->begin(), buffer.end()->end(), Buffer.begin()->begin(), &swapBytes<Sample>);
      File.write(safe_ptr_cast<const char*>(&Buffer[0]), static_cast<std::streamsize>(sizeInBytes));
#else
      File.write(safe_ptr_cast<const char*>(&buffer[0]), static_cast<std::streamsize>(sizeInBytes));
#endif
      Format.Size += static_cast<uint32_t>(sizeInBytes);
      Format.DataSize += static_cast<uint32_t>(sizeInBytes);
    }
  private:
    std::ofstream File;
    WaveFormat Format;
#ifdef BOOST_BIG_ENDIAN
    std::vector<MultiSample> Buffer;
#endif
  };
  
  Backend::Ptr WAVBackendCreator()
  {
    return Backend::Ptr(new SafeBackendWrapper<WAVBackend>());
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWAVBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, WAVBackendCreator, BACKEND_PRIORITY_LOW);
    }
  }
}
