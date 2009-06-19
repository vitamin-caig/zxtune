#include "../backend_enumerator.h"
#include "../sound_backend_impl.h"
#include "../sound_backend_types.h"

#include <tools.h>

#include <module.h>
#include <module_attrs.h>
#include <sound_attrs.h>
#include <../../lib/io/location.h>

#include <boost/noncopyable.hpp>

#include <cassert>
#include <fstream>
#include <iostream>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;
  //TODO
  const String::value_type TEXT_FILE_BACKEND_DESCRIPTON[] = "File output backend";
  const String::value_type FILE_BACKEND_KEY[] = {'f', 'i', 'l', 'e', 0};

  const String::value_type TEMPLATE_LASTNAME[] = {'$', '1', 0};
  const String::value_type TEMPLATE_FIRSTNAME[] = {'$', '2', 0};
  const String::value_type DEFAULT_FILE_NAME[] = {'$', '1', '.', 'w', 'a', 'v', 0};

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

  template<class T, class F>
  void Assign(T& dst, F src)
  {
    dst = fromLE(static_cast<T>(src));
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
        Params.DriverParameters = DEFAULT_FILE_NAME;
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
        GetModuleInfo(info);
        StringMap::const_iterator it(info.Properties.find(Module::ATTR_FILENAME));
        assert(it != info.Properties.end());
        StringArray subpathes;
        IO::SplitPath(it->second, subpathes);
        //cut filename
        String& fname(subpathes.front());
        const String::size_type dpos(fname.find_last_of("\\/"));
        if (String::npos != dpos)
        {
          fname = fname.substr(dpos + 1);
        }
        String filename(Params.DriverParameters);
        boost::algorithm::replace_all(filename, TEMPLATE_LASTNAME, subpathes.back());
        boost::algorithm::replace_all(filename, TEMPLATE_FIRSTNAME, fname);
        File.open(filename.c_str(), std::ios::binary);
        assert(File.is_open());
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
    info.Description = TEXT_FILE_BACKEND_DESCRIPTON;
    info.Key = FILE_BACKEND_KEY;
  }

  Backend::Ptr Creator()
  {
    return Backend::Ptr(new FileBackend);
  }

  BackendAutoRegistrator registrator(Creator, Descriptor);
}
