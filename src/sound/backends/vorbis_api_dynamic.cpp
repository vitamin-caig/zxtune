/*
Abstract:
  Vorbis api dynamic gate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "vorbis_api.h"
//common includes
#include <debug_log.h>
#include <tools.h>
//library includes
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune::Sound::Vorbis;

  class VorbisName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "vorbis";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libvorbis.so.0",
      };
      return std::vector<std::string>(ALTERNATIVES, ArrayEnd(ALTERNATIVES));
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      return std::vector<std::string>();
    }
  };

  const Debug::Stream Dbg("Sound::Backend::Ogg");

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(lib)
    {
      Dbg("Library loaded");
    }

    virtual ~DynamicApi()
    {
      Dbg("Library unloaded");
    }

    
    virtual int vorbis_block_clear(vorbis_block *vb)
    {
      static const char NAME[] = "vorbis_block_clear";
      typedef int ( *FunctionType)(vorbis_block *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vb);
    }
    
    virtual int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb)
    {
      static const char NAME[] = "vorbis_block_init";
      typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_block *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v, vb);
    }
    
    virtual void vorbis_dsp_clear(vorbis_dsp_state *v)
    {
      static const char NAME[] = "vorbis_dsp_clear";
      typedef void ( *FunctionType)(vorbis_dsp_state *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v);
    }
    
    virtual void vorbis_info_clear(vorbis_info *vi)
    {
      static const char NAME[] = "vorbis_info_clear";
      typedef void ( *FunctionType)(vorbis_info *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vi);
    }
    
    virtual void vorbis_info_init(vorbis_info *vi)
    {
      static const char NAME[] = "vorbis_info_init";
      typedef void ( *FunctionType)(vorbis_info *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vi);
    }
    
    virtual const char *vorbis_version_string(void)
    {
      static const char NAME[] = "vorbis_version_string";
      typedef const char *( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual int vorbis_analysis(vorbis_block *vb, ogg_packet *op)
    {
      static const char NAME[] = "vorbis_analysis";
      typedef int ( *FunctionType)(vorbis_block *, ogg_packet *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vb, op);
    }
    
    virtual int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb)
    {
      static const char NAME[] = "vorbis_analysis_blockout";
      typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_block *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v, vb);
    }
    
    virtual float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals)
    {
      static const char NAME[] = "vorbis_analysis_buffer";
      typedef float** ( *FunctionType)(vorbis_dsp_state *, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v, vals);
    }
    
    virtual int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code)
    {
      static const char NAME[] = "vorbis_analysis_headerout";
      typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_comment *, ogg_packet *, ogg_packet *, ogg_packet *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v, vc, op, op_comm, op_code);
    }
    
    virtual int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi)
    {
      static const char NAME[] = "vorbis_analysis_init";
      typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_info *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v, vi);
    }
    
    virtual int vorbis_analysis_wrote(vorbis_dsp_state *v,int vals)
    {
      static const char NAME[] = "vorbis_analysis_wrote";
      typedef int ( *FunctionType)(vorbis_dsp_state *, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(v, vals);
    }
    
    virtual int vorbis_bitrate_addblock(vorbis_block *vb)
    {
      static const char NAME[] = "vorbis_bitrate_addblock";
      typedef int ( *FunctionType)(vorbis_block *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vb);
    }
    
    virtual int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op)
    {
      static const char NAME[] = "vorbis_bitrate_flushpacket";
      typedef int ( *FunctionType)(vorbis_dsp_state *, ogg_packet *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vd, op);
    }
    
    virtual void vorbis_comment_add_tag(vorbis_comment *vc, const char *tag, const char *contents)
    {
      static const char NAME[] = "vorbis_comment_add_tag";
      typedef void ( *FunctionType)(vorbis_comment *, const char *, const char *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vc, tag, contents);
    }
    
    virtual void vorbis_comment_clear(vorbis_comment *vc)
    {
      static const char NAME[] = "vorbis_comment_clear";
      typedef void ( *FunctionType)(vorbis_comment *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vc);
    }
    
    virtual void vorbis_comment_init(vorbis_comment *vc)
    {
      static const char NAME[] = "vorbis_comment_init";
      typedef void ( *FunctionType)(vorbis_comment *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vc);
    }
    
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

}

namespace ZXTune
{
  namespace Sound
  {
    namespace Vorbis
    {
      Api::Ptr LoadDynamicApi()
      {
        static const VorbisName NAME;
        const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
        return boost::make_shared<DynamicApi>(lib);
      }
    }
  }
}
