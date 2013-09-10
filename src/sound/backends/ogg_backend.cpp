/*
Abstract:
  Ogg file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ogg_api.h"
#include "vorbis_api.h"
#include "vorbisenc_api.h"
#include "storage.h"
#include "file_backend.h"
//common includes
#include <error_tools.h>
#include <tools.h>
//library includes
#include <binary/data_adapter.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG B01A305D

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Ogg");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace Ogg
{
  const String ID = Text::OGG_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("OGG support backend");

  const uint_t BITRATE_MIN = 48;
  const uint_t BITRATE_MAX = 500;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 10;

  void CheckVorbisCall(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, translate("Error in OGG backend: code %1%."), res);
    }
  }

  class OggBitStream
  {
  public:
    typedef boost::shared_ptr<OggBitStream> Ptr;
    OggBitStream(Api::Ptr api, Binary::OutputStream::Ptr file)
      : OggApi(api)
      , File(file)
    {
      CheckVorbisCall(OggApi->ogg_stream_init(&Stream, std::time(0)), THIS_LINE);
    }

    ~OggBitStream()
    {
      OggApi->ogg_stream_clear(&Stream);
    }

    void AddPacket(ogg_packet* pkt)
    {
      CheckVorbisCall(OggApi->ogg_stream_packetin(&Stream, pkt), THIS_LINE);
    }

    void Save()
    {
      ogg_page page;
      while (const int streamRes = OggApi->ogg_stream_pageout(&Stream, &page))
      {
        WritePage(page);
        if (OggApi->ogg_page_eos(&page))
        {
          break;
        }
      }
    }

    void Flush()
    {
      ogg_page page;
      while (const int streamRes = OggApi->ogg_stream_flush(&Stream, &page))
      {
        WritePage(page);
      }
    }
  private:
    void WritePage(const ogg_page& page)
    {
      File->ApplyData(Binary::DataAdapter(page.header, page.header_len));
      File->ApplyData(Binary::DataAdapter(page.body, page.body_len));
    }
  private:
    const Api::Ptr OggApi;
    const Binary::OutputStream::Ptr File;
    ogg_stream_state Stream;
  };

  class MetaData
  {
  public:
    typedef boost::shared_ptr<MetaData> Ptr;

    explicit MetaData(Vorbis::Api::Ptr api)
      : VorbisApi(api)
    {
      VorbisApi->vorbis_comment_init(&Data);
    }

    ~MetaData()
    {
      VorbisApi->vorbis_comment_clear(&Data);
    }

    void AddTag(const String& name, const String& value)
    {
      const std::string nameC = name;//TODO
      const std::string valueC = value;//TODO
      VorbisApi->vorbis_comment_add_tag(&Data, nameC.c_str(), valueC.c_str());
    }

    vorbis_comment* Get()
    {
      return &Data;
    }
  private:
    const Vorbis::Api::Ptr VorbisApi;
    vorbis_comment Data;
  };

  class VorbisState
  {
  public:
    typedef boost::shared_ptr<VorbisState> Ptr;
    VorbisState(Vorbis::Api::Ptr api, vorbis_info* info)
      : VorbisApi(api)
    {
      VorbisApi->vorbis_analysis_init(&State, info);
      VorbisApi->vorbis_block_init(&State, &Block);
    }

    ~VorbisState()
    {
      VorbisApi->vorbis_block_clear(&Block);
      VorbisApi->vorbis_dsp_clear(&State);
    }

    void Encode(const Chunk& data)
    {
      const std::size_t samples = data.size();
      float** const buffer = VorbisApi->vorbis_analysis_buffer(&State, samples);
      for (std::size_t pos = 0; pos != samples; ++pos)
      {
        const Sample in = data[pos];
        buffer[0][pos] = ToFloat(in.Left());
        buffer[1][pos] = ToFloat(in.Right());
      }
      CheckVorbisCall(VorbisApi->vorbis_analysis_wrote(&State, samples), THIS_LINE);
    }

    void Flush()
    {
      CheckVorbisCall(VorbisApi->vorbis_analysis_wrote(&State, 0), THIS_LINE);
    }

    void Save(OggBitStream& stream)
    {
      ogg_packet packet;
      while (const int blkRes = VorbisApi->vorbis_analysis_blockout(&State, &Block))
      {
        CheckVorbisCall(blkRes, THIS_LINE);
        CheckVorbisCall(VorbisApi->vorbis_analysis(&Block, 0), THIS_LINE);
        CheckVorbisCall(VorbisApi->vorbis_bitrate_addblock(&Block), THIS_LINE);
        while (const int pcktRes = VorbisApi->vorbis_bitrate_flushpacket(&State, &packet))
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
    //from SAMPLE_MIN..SAMPLE_MAX scale to -1.0..+1.0
    static float ToFloat(Sample::Type smp)
    {
      return float(int_t(smp) - Sample::MID) / (Sample::MAX - Sample::MID);
    }
  private:
    const Vorbis::Api::Ptr VorbisApi;
    vorbis_dsp_state State;
    vorbis_block Block;
  };

  class VorbisInfo
  {
  public:
    typedef boost::shared_ptr<VorbisInfo> Ptr;
    VorbisInfo(Vorbis::Api::Ptr vorbisApi, VorbisEnc::Api::Ptr vorbisEncApi)
      : VorbisApi(vorbisApi)
      , VorbisEncApi(vorbisEncApi)
    {
      VorbisApi->vorbis_info_init(&Data);
    }

    ~VorbisInfo()
    {
      VorbisApi->vorbis_info_clear(&Data);
    }

    void SetQuality(uint_t quality, uint_t samplerate)
    {
      CheckVorbisCall(VorbisEncApi->vorbis_encode_init_vbr(&Data, Sample::CHANNELS, samplerate, float(quality) / 10), THIS_LINE);
    }

    void SetABR(uint_t bitrate, uint_t samplerate)
    {
      CheckVorbisCall(VorbisEncApi->vorbis_encode_init(&Data, Sample::CHANNELS, samplerate, -1, bitrate * 1000, -1), THIS_LINE);
    }

    vorbis_info* Get()
    {
      return &Data;
    }
  private:
    const Vorbis::Api::Ptr VorbisApi;
    const VorbisEnc::Api::Ptr VorbisEncApi;
    vorbis_info Data;
  };

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(Vorbis::Api::Ptr api, VorbisInfo::Ptr info, OggBitStream::Ptr stream)
      : VorbisApi(api)
      , Info(info)
      , Meta(boost::make_shared<MetaData>(VorbisApi))
      , State(boost::make_shared<VorbisState>(VorbisApi, Info->Get()))
      , Stream(stream)
    {
      Dbg("Stream initialized");
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
      CheckVorbisCall(VorbisApi->vorbis_analysis_headerout(State->Get(), Meta->Get(), &id, &meta, &code), THIS_LINE);
      Stream->AddPacket(&id);
      Stream->AddPacket(&meta);
      Stream->AddPacket(&code);
      Stream->Flush();
    }

    virtual void ApplyData(const Chunk::Ptr& data)
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
    const Vorbis::Api::Ptr VorbisApi;
    const VorbisInfo::Ptr Info;
    const MetaData::Ptr Meta;
    const VorbisState::Ptr State;
    const OggBitStream::Ptr Stream;
  };

  class StreamParameters
  {
  public:
    explicit StreamParameters(Parameters::Accessor::Ptr params)
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
        throw MakeFormattedError(THIS_LINE,
          translate("OGG backend error: invalid mode '%1%'."), mode);
      }
    }

    uint_t GetBitrate() const
    {
      Parameters::IntType bitrate = Parameters::ZXTune::Sound::Backends::Ogg::BITRATE_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Ogg::BITRATE, bitrate) &&
        !Math::InRange<Parameters::IntType>(bitrate, BITRATE_MIN, BITRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("OGG backend error: bitrate (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(bitrate), BITRATE_MIN, BITRATE_MAX);
      }
      return static_cast<uint_t>(bitrate);
    }

    uint_t GetQuality() const
    {
      Parameters::IntType quality = Parameters::ZXTune::Sound::Backends::Ogg::QUALITY_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Ogg::QUALITY, quality) &&
        !Math::InRange<Parameters::IntType>(quality, QUALITY_MIN, QUALITY_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("OGG backend error: quality (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(quality), QUALITY_MIN, QUALITY_MAX);
      }
      return static_cast<uint_t>(quality);
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    FileStreamFactory(Api::Ptr oggApi, Vorbis::Api::Ptr vorbisApi, VorbisEnc::Api::Ptr vorbisEncApi, Parameters::Accessor::Ptr params)
      : OggApi(oggApi)
      , VorbisApi(vorbisApi)
      , VorbisEncApi(vorbisEncApi)
      , Params(params)
    {
    }

    virtual String GetId() const
    {
      return ID;
    }

    virtual FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const
    {
      const VorbisInfo::Ptr info = boost::make_shared<VorbisInfo>(VorbisApi, VorbisEncApi);
      SetupInfo(*info);
      const OggBitStream::Ptr bitStream = boost::make_shared<OggBitStream>(OggApi, stream);
      return boost::make_shared<FileStream>(VorbisApi, info, bitStream);
    }
  private:
    void SetupInfo(VorbisInfo& info) const
    {
      const StreamParameters stream(Params);
      const RenderParameters::Ptr sound = RenderParameters::Create(Params);

      const uint_t samplerate = sound->SoundFreq();
      Dbg("Samplerate is %1%", samplerate);
      if (stream.IsABRMode())
      {
        const uint_t bitrate = stream.GetBitrate();
        Dbg("Setting ABR to %1%kbps", bitrate);
        info.SetABR(bitrate, samplerate);
      }
      else
      {
        const uint_t quality = stream.GetQuality();
        Dbg("Setting VBR quality to %1%", quality);
        info.SetQuality(quality, samplerate);
      }

    }
  private:
    const Api::Ptr OggApi;
    const Vorbis::Api::Ptr VorbisApi;
    const VorbisEnc::Api::Ptr VorbisEncApi;
    const Parameters::Accessor::Ptr Params;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    BackendWorkerFactory(Api::Ptr oggApi, Vorbis::Api::Ptr vorbisApi, VorbisEnc::Api::Ptr vorbisEncApi)
      : OggApi(oggApi)
      , VorbisApi(vorbisApi)
      , VorbisEncApi(vorbisEncApi)
    {
    }

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params) const
    {
      const FileStreamFactory::Ptr factory = boost::make_shared<FileStreamFactory>(OggApi, VorbisApi, VorbisEncApi, params);
      return CreateFileBackendWorker(params, factory);
    }
  private:
    const Api::Ptr OggApi;
    const Vorbis::Api::Ptr VorbisApi;
    const VorbisEnc::Api::Ptr VorbisEncApi;
  };
}//Ogg
}//Sound

namespace Sound
{
  void RegisterOggBackend(BackendsStorage& storage)
  {
    try
    {
      const Ogg::Api::Ptr oggApi = Ogg::LoadDynamicApi();
      const Vorbis::Api::Ptr vorbisApi = Vorbis::LoadDynamicApi();
      const VorbisEnc::Api::Ptr vorbisEncApi = VorbisEnc::LoadDynamicApi();
      Dbg("Detected Vorbis library %1%", vorbisApi->vorbis_version_string());
      const BackendWorkerFactory::Ptr factory = boost::make_shared<Ogg::BackendWorkerFactory>(oggApi, vorbisApi, vorbisEncApi);
      storage.Register(Ogg::ID, Ogg::DESCRIPTION, CAP_TYPE_FILE, factory);
    }
    catch (const Error& e)
    {
      storage.Register(Ogg::ID, Ogg::DESCRIPTION, CAP_TYPE_FILE, e);
    }
  }
}

