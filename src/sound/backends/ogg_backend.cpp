/*
Abstract:
  OGG file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef OGG_SUPPORT

//local includes
#include "enumerator.h"
#include "file_backend.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <shared_library_gate.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//platform-specific includes
#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG B01A305D

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Ogg");

  const Char OGG_BACKEND_ID[] = {'o', 'g', 'g', 0};

  const uint_t BITRATE_MIN = 48;
  const uint_t BITRATE_MAX = 500;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 10;

  struct VorbisEncLibraryTraits
  {
    static std::string GetName()
    {
      return "vorbisenc";
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };

  typedef SharedLibraryGate<VorbisEncLibraryTraits> VorbisEncLibrary;

  struct VorbisLibraryTraits
  {
    static std::string GetName()
    {
      return "vorbis";
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };

  typedef SharedLibraryGate<VorbisLibraryTraits> VorbisLibrary;

  struct OggLibraryTraits
  {
    static std::string GetName()
    {
      return "ogg";
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };

  typedef SharedLibraryGate<OggLibraryTraits> OggLibrary;

  void CheckVorbisCall(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_OGG_BACKEND_ERROR, res);
    }
  }

  class OggBitStream
  {
  public:
    typedef boost::shared_ptr<OggBitStream> Ptr;
    explicit OggBitStream(std::auto_ptr<std::ofstream> file)
      : File(file)
    {
      CheckVorbisCall(::ogg_stream_init(&Stream, std::time(0)), THIS_LINE);
    }

    ~OggBitStream()
    {
      ::ogg_stream_clear(&Stream);
    }

    void AddPacket(ogg_packet* pkt)
    {
      CheckVorbisCall(::ogg_stream_packetin(&Stream, pkt), THIS_LINE);
    }

    void Save()
    {
      ogg_page page;
      while (const int streamRes = ::ogg_stream_pageout(&Stream, &page))
      {
        WritePage(page);
        if (::ogg_page_eos(&page))
        {
          break;
        }
      }
    }

    void Flush()
    {
      ogg_page page;
      while (const int streamRes = ::ogg_stream_flush(&Stream, &page))
      {
        WritePage(page);
      }
    }
  private:
    void WritePage(const ogg_page& page)
    {
      File->write(safe_ptr_cast<const char*>(page.header), page.header_len);
      File->write(safe_ptr_cast<const char*>(page.body), page.body_len);
    }
  private:
    const std::auto_ptr<std::ofstream> File;
    ogg_stream_state Stream;
  };

  class VorbisMeta
  {
  public:
    typedef boost::shared_ptr<VorbisMeta> Ptr;
    VorbisMeta()
    {
      ::vorbis_comment_init(&Data);
    }

    ~VorbisMeta()
    {
      ::vorbis_comment_clear(&Data);
    }

    void AddTag(const String& name, const String& value)
    {
      const std::string nameC = IO::ConvertToFilename(name);
      const std::string valueC = IO::ConvertToFilename(value);
      ::vorbis_comment_add_tag(&Data, nameC.c_str(), valueC.c_str());
    }

    vorbis_comment* Get()
    {
      return &Data;
    }
  private:
    vorbis_comment Data;
  };

  class VorbisState
  {
  public:
    typedef boost::shared_ptr<VorbisState> Ptr;
    explicit VorbisState(vorbis_info* info)
    {
      ::vorbis_analysis_init(&State, info);
      ::vorbis_block_init(&State, &Block);
    }

    ~VorbisState()
    {
      ::vorbis_block_clear(&Block);
      ::vorbis_dsp_clear(&State);
    }

    void Encode(const Chunk& data)
    {
      const std::size_t samples = data.size();
      float** const buffer = ::vorbis_analysis_buffer(&State, samples);
      for (uint_t chan = 0; chan != OUTPUT_CHANNELS; ++chan)
      {
        std::transform(data.begin(), data.end(), buffer[chan], std::bind2nd(std::ptr_fun(&SampleToFloat), chan));
      }
      CheckVorbisCall(::vorbis_analysis_wrote(&State, samples), THIS_LINE);
    }

    void Flush()
    {
      CheckVorbisCall(::vorbis_analysis_wrote(&State, 0), THIS_LINE);
    }

    void Save(OggBitStream& stream)
    {
      ogg_packet packet;
      while (const int blkRes = ::vorbis_analysis_blockout(&State, &Block))
      {
        CheckVorbisCall(blkRes, THIS_LINE);
        CheckVorbisCall(::vorbis_analysis(&Block, 0), THIS_LINE);
        CheckVorbisCall(::vorbis_bitrate_addblock(&Block), THIS_LINE);
        while (const int pcktRes = ::vorbis_bitrate_flushpacket(&State, &packet))
        {
          stream.AddPacket(&packet);
          stream.Save();
        }
      }
    }

    vorbis_dsp_state* Get()
    {
      return &State;
    }
  private:
    static float SampleToFloat(MultiSample smp, uint_t channel)
    {
      return float(smp[channel]) / SAMPLE_MAX;
    }
  private:
    vorbis_dsp_state State;
    vorbis_block Block;
  };

  class VorbisInfo
  {
  public:
    typedef boost::shared_ptr<VorbisInfo> Ptr;
    VorbisInfo()
    {
      ::vorbis_info_init(&Data);
    }

    ~VorbisInfo()
    {
      ::vorbis_info_clear(&Data);
    }

    void SetQuality(uint_t quality, uint_t samplerate)
    {
      CheckVorbisCall(::vorbis_encode_init_vbr(&Data, OUTPUT_CHANNELS, samplerate, float(quality) / 10), THIS_LINE);
    }

    void SetABR(uint_t bitrate, uint_t samplerate)
    {
      CheckVorbisCall(::vorbis_encode_init(&Data, OUTPUT_CHANNELS, samplerate, -1, bitrate * 1000, -1), THIS_LINE);
    }

    vorbis_info* Get()
    {
      return &Data;
    }
  private:
    vorbis_info Data;
  };

  class OggVorbisStream : public FileStream
  {
  public:
    OggVorbisStream(VorbisInfo::Ptr info, OggBitStream::Ptr stream)
      : Info(info)
      , Meta(boost::make_shared<VorbisMeta>())
      , State(boost::make_shared<VorbisState>(Info->Get()))
      , Stream(stream)
    {
      Log::Debug(THIS_MODULE, "Stream initialized");
    }

    virtual void SetTitle(const String& title)
    {
      Meta->AddTag(Text::OGG_BACKEND_TITLE_TAG, title);
    }

    virtual void SetAuthor(const String& author)
    {
      Meta->AddTag(Text::OGG_BACKEND_AUTHOR_TAG, author);
    }

    virtual void SetComment(const String& comment)
    {
      Meta->AddTag(Text::OGG_BACKEND_COMMENT_TAG, comment);
    }

    virtual void FlushMetadata()
    {
      ogg_packet id, meta, code;
      CheckVorbisCall(::vorbis_analysis_headerout(State->Get(), Meta->Get(), &id, &meta, &code), THIS_LINE);
      Stream->AddPacket(&id);
      Stream->AddPacket(&meta);
      Stream->AddPacket(&code);
      Stream->Flush();
    }

    virtual void ApplyData(const ChunkPtr& data)
    {
      State->Encode(*data);
      State->Save(*Stream);
    }

    virtual void Flush()
    {
      State->Flush();
      State->Save(*Stream);
    }
  private:
    const VorbisInfo::Ptr Info;
    const VorbisMeta::Ptr Meta;
    const VorbisState::Ptr State;
    const OggBitStream::Ptr Stream;
  };

  class OggParameters
  {
  public:
    explicit OggParameters(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    bool IsABRMode() const
    {
      Parameters::StringType mode = Parameters::ZXTune::Sound::Backends::Ogg::MODE_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Sound::Backends::Ogg::MODE, mode);
      if (mode == Parameters::ZXTune::Sound::Backends::Ogg::MODE_ABR)
      {
        return true;
      }
      else if (mode == Parameters::ZXTune::Sound::Backends::Ogg::MODE_QUALITY)
      {
        return false;
      }
      else
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_OGG_BACKEND_INVALID_MODE, mode);
      }
    }

    uint_t GetBitrate() const
    {
      Parameters::IntType bitrate = Parameters::ZXTune::Sound::Backends::Ogg::BITRATE_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Ogg::BITRATE, bitrate) &&
        !in_range<Parameters::IntType>(bitrate, BITRATE_MIN, BITRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_OGG_BACKEND_INVALID_BITRATE, static_cast<int_t>(bitrate), BITRATE_MIN, BITRATE_MAX);
      }
      return static_cast<uint_t>(bitrate);
    }

    uint_t GetQuality() const
    {
      Parameters::IntType quality = Parameters::ZXTune::Sound::Backends::Ogg::QUALITY_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Ogg::QUALITY, quality) &&
        !in_range<Parameters::IntType>(quality, QUALITY_MIN, QUALITY_MAX))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_OGG_BACKEND_INVALID_QUALITY, static_cast<int_t>(quality), QUALITY_MIN, QUALITY_MAX);
      }
      return static_cast<uint_t>(quality);
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class OggFileFactory : public FileStreamFactory
  {
  public:
    explicit OggFileFactory(Parameters::Accessor::Ptr params)
      : Params(params)
      , RenderingParameters(RenderParameters::Create(params))
    {
    }

    virtual String GetId() const
    {
      return OGG_BACKEND_ID;
    }

    virtual FileStream::Ptr OpenStream(const String& fileName, bool overWrite) const
    {
      std::auto_ptr<std::ofstream> rawFile = IO::CreateFile(fileName, overWrite);
      const VorbisInfo::Ptr info = boost::make_shared<VorbisInfo>();
      SetupInfo(*info);
      const OggBitStream::Ptr stream(new OggBitStream(rawFile));
      return FileStream::Ptr(new OggVorbisStream(info, stream));
    }
  private:
    void SetupInfo(VorbisInfo& info) const
    {
      const uint_t samplerate = RenderingParameters->SoundFreq();
      Log::Debug(THIS_MODULE, "Samplerate is %1%", samplerate);
      if (Params.IsABRMode())
      {
        const uint_t bitrate = Params.GetBitrate();
        Log::Debug(THIS_MODULE, "Setting ABR to %1%kbps", bitrate);
        info.SetABR(bitrate, samplerate);
      }
      else
      {
        const uint_t quality = Params.GetQuality();
        Log::Debug(THIS_MODULE, "Setting VBR quality to %1%", quality);
        info.SetQuality(quality, samplerate);
      }

    }
  private:
    const OggParameters Params;
    const RenderParameters::Ptr RenderingParameters;
  };

  class OGGBackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return OGG_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::OGG_BACKEND_DESCRIPTION;
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
        const FileStreamFactory::Ptr factory = boost::make_shared<OggFileFactory>(allParams);
        const BackendWorker::Ptr worker = CreateFileBackendWorker(allParams, factory);
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
    void RegisterOGGBackend(BackendsEnumerator& enumerator)
    {
      if (VorbisEncLibrary::Instance().IsAccessible() &&
          VorbisLibrary::Instance().IsAccessible() &&
          OggLibrary::Instance().IsAccessible())
      {
        Log::Debug(THIS_MODULE, "Detected Vorbis library %1%", ::vorbis_version_string());
        const BackendCreator::Ptr creator(new OGGBackendCreator());
        enumerator.RegisterCreator(creator);
      }
    }
  }
}

//global namespace
#define STR(a) #a

#define VORBISENCODE_FUNC(func) VorbisEncLibrary::Instance().GetSymbol(&func, STR(func))
#define VORBIS_FUNC(func)  VorbisLibrary::Instance().GetSymbol(&func, STR(func))
#define OGG_FUNC(func) OggLibrary::Instance().GetSymbol(&func, STR(func))

int vorbis_encode_init(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate)
{
  return VORBISENCODE_FUNC(vorbis_encode_init)(vi, channels, rate, max_bitrate, nominal_bitrate, min_bitrate);
}

int vorbis_encode_init_vbr(vorbis_info *vi, long channels, long rate, float base_quality)
{
  return VORBISENCODE_FUNC(vorbis_encode_init_vbr)(vi, channels, rate, base_quality);
}

int vorbis_block_clear(vorbis_block *vb)
{
  return VORBIS_FUNC(vorbis_block_clear)(vb);
}

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb)
{
  return VORBIS_FUNC(vorbis_block_init)(v, vb);
}

void vorbis_dsp_clear(vorbis_dsp_state *v)
{
  return VORBIS_FUNC(vorbis_dsp_clear)(v);
}

void vorbis_info_clear(vorbis_info *vi)
{
  return VORBIS_FUNC(vorbis_info_clear)(vi);
}

void vorbis_info_init(vorbis_info *vi)
{
  return VORBIS_FUNC(vorbis_info_init)(vi);
}

const char *vorbis_version_string(void)
{
  return VORBIS_FUNC(vorbis_version_string)();
}

int vorbis_analysis(vorbis_block *vb, ogg_packet *op)
{
  return VORBIS_FUNC(vorbis_analysis)(vb, op);
}

int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb)
{
  return VORBIS_FUNC(vorbis_analysis_blockout)(v, vb);
}

float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals)
{
  return VORBIS_FUNC(vorbis_analysis_buffer)(v, vals);
}

int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code)
{
  return VORBIS_FUNC(vorbis_analysis_headerout)(v, vc, op, op_comm, op_code);
}

int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi)
{
  return VORBIS_FUNC(vorbis_analysis_init)(v, vi);
}

int vorbis_analysis_wrote(vorbis_dsp_state *v,int vals)
{
  return VORBIS_FUNC(vorbis_analysis_wrote)(v, vals);
}

int vorbis_bitrate_addblock(vorbis_block *vb)
{
  return VORBIS_FUNC(vorbis_bitrate_addblock)(vb);
}

int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op)
{
  return VORBIS_FUNC(vorbis_bitrate_flushpacket)(vd, op);
}

void vorbis_comment_add_tag(vorbis_comment *vc, const char *tag, const char *contents)
{
  return VORBIS_FUNC(vorbis_comment_add_tag)(vc, tag, contents);
}

void vorbis_comment_clear(vorbis_comment *vc)
{
  return VORBIS_FUNC(vorbis_comment_clear)(vc);
}

void vorbis_comment_init(vorbis_comment *vc)
{
  return VORBIS_FUNC(vorbis_comment_init)(vc);
}

int ogg_stream_init(ogg_stream_state *os, int serialno)
{
  return OGG_FUNC(ogg_stream_init)(os, serialno);
}

int ogg_stream_clear(ogg_stream_state *os)
{
  return OGG_FUNC(ogg_stream_clear)(os);
}

int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op)
{
  return OGG_FUNC(ogg_stream_packetin)(os, op);
}

int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og)
{
  return OGG_FUNC(ogg_stream_pageout)(os, og);
}

int ogg_stream_flush(ogg_stream_state *os, ogg_page *og)
{
  return OGG_FUNC(ogg_stream_flush)(os, og);
}

int ogg_page_eos(const ogg_page *og)
{
  return OGG_FUNC(ogg_page_eos)(og);
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterOGGBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}
#endif
