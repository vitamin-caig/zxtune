#include "../backend_enumerator.h"
#include "../sound_backend_impl.h"
#include "../sound_backend_types.h"

#include <error.h>
#include <tools.h>

#include <module.h>
#include <module_attrs.h>
#include <sound_attrs.h>
#include <../../lib/io/location.h>

#include <boost/noncopyable.hpp>

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <text/errors.h>
#include <text/backends.h>

#define FILE_TAG EF5CB4C6

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

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

  const String::value_type STDIN_NAME[] = {'-', '\0'};

#ifdef BOOST_BIG_ENDIAN
  //helper
  inline Sample SwapSample(Sample data)
  {
    return swapBytes(data);
  }
#endif

  void CutFilename(String& fname)
  {
    const String::size_type dpos(fname.find_last_of("\\/"));//TODO
    if (String::npos != dpos)
    {
      fname = fname.substr(dpos + 1);
    }
  }

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
        StringArray subpathes;
        IO::SplitPath(it->second, subpathes);
        CutFilename(subpathes.front());
        infoFile << fmt % Module::ATTR_FILENAME % subpathes.back();
        if (subpathes.size() != 1)
        {
          infoFile << fmt % Module::ATTR_ALBUM % subpathes.front();
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

  class FileBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    FileBackend() : Stream(0), File(), RawOutput(false)
    {
      std::memcpy(Format.Id, RIFF, sizeof(RIFF));
      std::memcpy(Format.Type, WAVEfmt, sizeof(WAVEfmt));
      //??? TODO
      Format.ChunkSize = fromLE<uint32_t>(16);
      Format.Compression = fromLE<uint16_t>(1);//PCM
      Format.Channels = fromLE<uint16_t>(OUTPUT_CHANNELS);
      std::memcpy(Format.DataId, DATA, sizeof(DATA));
    }

    virtual ~FileBackend()
    {
      assert((!Stream && !File.is_open()) || !"FileBackend::Stop should be called before exit");
    }

    virtual void GetInfo(Info& info) const
    {
      return Descriptor(info);
    }

    virtual void OnParametersChanged(unsigned /*changedFields*/)
    {
      //loop is disabled
      Params.SoundParameters.Flags &= ~MOD_LOOP;
      const bool needStartup(Stream != 0);
      if (needStartup)
      {
        OnShutdown();
      }
      //force raw mode if stdout
      RawOutput = Params.DriverParameters == STDIN_NAME ||
        (Params.DriverFlags & RAW_STREAM);
      if (!RawOutput && Params.DriverParameters.empty())
      {
        Params.DriverParameters = TEXT_DEFAULT_FILENAME_TEMPLATE;
      }
      if (needStartup)
      {
        OnStartup();
      }
    }

    virtual void OnStartup()
    {
      assert(!Stream);
      if (RawOutput)
      {
        //not unicode or smth version
        Stream = &std::cout;
      }
      else
      {
        Module::Information info;
        assert(Player);
        Player->GetModuleInfo(info);
        StringArray subpathes;
        IO::SplitPath(info.Properties[Module::ATTR_FILENAME], subpathes);
        CutFilename(subpathes.front());
        String filename(Params.DriverParameters);
        boost::algorithm::replace_all(filename, String(TEXT_LASTNAME_TEMPLATE), subpathes.back());
        boost::algorithm::replace_all(filename, String(TEXT_FIRSTNAME_TEMPLATE), subpathes.front());
        StringMap::const_iterator it(info.Properties.find(Module::ATTR_CRC));
        if (it != info.Properties.end())
        {
          boost::algorithm::replace_all(filename, String(TEXT_CRC_TEMPLATE), it->second);
        }
        if (!RawOutput && (Params.DriverFlags & ANNOTATE_STREAM))
        {
          //do annotation
          Annotate(info, Params, filename + FILE_ANNOTATION_EXT);
        }
        File.open(filename.c_str(), std::ios::binary);
        if (!File.is_open())
        {
          throw MakeFormattedError(ERROR_DETAIL, 1, TEXT_ERROR_OPEN_FILE, filename);
        }
        File.seekp(sizeof(Format));
        Stream = &File;
      }

      //??? TODO
      Format.Samplerate = fromLE<uint32_t>(Params.SoundParameters.SoundFreq);
      Format.BytesPerSec = fromLE<uint32_t>(Params.SoundParameters.SoundFreq * sizeof(SampleArray));
      Format.Align = fromLE<uint16_t>(sizeof(SampleArray));
      Format.BitsPerSample = fromLE<uint16_t>(8 * sizeof(Sample));
      //swap on final
      Format.Size =  sizeof(Format) - 8;
      Format.DataSize = 0;
    }

    virtual void OnShutdown()
    {
      if (File.is_open())
      {
        if (!RawOutput)
        {
          File.seekp(0);
          //??? TODO
          Format.Size = fromLE(Format.Size);
          Format.DataSize = fromLE(Format.DataSize);
          File.write(safe_ptr_cast<const char*>(&Format), sizeof(Format));
        }
        File.close();
      }
      Stream = 0;
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
      assert(Stream);
#ifdef BOOST_BIG_ENDIAN
      if (sizeof(Sample) > 1)
      {
        assert(0 == sizeInBytes % sizeof(Sample));
        Buffer.resize(sizeInBytes / sizeof(Sample));
        const Sample* const sampleData(static_cast<const Sample*>(data));
        std::transform(sampleData, sampleData + sizeInBytes / sizeof(Sample), Buffer.begin(),
          &SwapSample);
        Stream->write(safe_ptr_cast<const char*>(&Buffer[0]), static_cast<std::streamsize>(sizeInBytes));
      }
      else
      {
        Stream->write(static_cast<const char*>(data), static_cast<std::streamsize>(sizeInBytes));
      }
#else
      Stream->write(static_cast<const char*>(data), static_cast<std::streamsize>(sizeInBytes));
#endif
      Format.Size += static_cast<uint32_t>(sizeInBytes);
      Format.DataSize += static_cast<uint32_t>(sizeInBytes);
    }
  private:
    std::ostream* Stream;
    std::ofstream File;
    bool RawOutput;
    WaveFormat Format;
#ifdef BOOST_BIG_ENDIAN
    std::vector<Sample> Buffer;
#endif
  };

  void Descriptor(Backend::Info& info)
  {
    info.Description = TEXT_FILE_BACKEND_DESCRIPTION;
    info.Key = FILE_BACKEND_KEY;
  }

  Backend::Ptr Creator()
  {
    return Backend::Ptr(new FileBackend);
  }

  BackendAutoRegistrator registrator(Creator, Descriptor);
}
