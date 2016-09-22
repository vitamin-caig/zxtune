/**
*
* @file
*
* @brief  ALSA subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "alsa_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace Alsa
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName()
      {
      }

      virtual std::string Base() const
      {
        return "asound";
      }
      
      virtual std::vector<std::string> PosixAlternatives() const
      {
        static const std::string ALTERNATIVES[] =
        {
          "libasound.so.2.0.0",//deb-based
          "libasound.so.2",    //rpm-based
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
      
      virtual std::vector<std::string> WindowsAlternatives() const
      {
        return std::vector<std::string>();
      }
    };

    const Debug::Stream Dbg("Sound::Backend::Alsa");

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

      
      virtual const char * snd_asoundlib_version (void)
      {
        static const char NAME[] = "snd_asoundlib_version";
        typedef const char * ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      virtual const char * snd_strerror (int errnum)
      {
        static const char NAME[] = "snd_strerror";
        typedef const char * ( *FunctionType)(int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(errnum);
      }
      
      virtual int snd_config_update_free_global (void)
      {
        static const char NAME[] = "snd_config_update_free_global";
        typedef int ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      virtual int snd_mixer_open (snd_mixer_t **mixer, int mode)
      {
        static const char NAME[] = "snd_mixer_open";
        typedef int ( *FunctionType)(snd_mixer_t **, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer, mode);
      }
      
      virtual int snd_mixer_close (snd_mixer_t *mixer)
      {
        static const char NAME[] = "snd_mixer_close";
        typedef int ( *FunctionType)(snd_mixer_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer);
      }
      
      virtual snd_mixer_elem_t * snd_mixer_first_elem (snd_mixer_t *mixer)
      {
        static const char NAME[] = "snd_mixer_first_elem";
        typedef snd_mixer_elem_t * ( *FunctionType)(snd_mixer_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer);
      }
      
      virtual int snd_mixer_attach (snd_mixer_t *mixer, const char *name)
      {
        static const char NAME[] = "snd_mixer_attach";
        typedef int ( *FunctionType)(snd_mixer_t *, const char *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer, name);
      }
      
      virtual int snd_mixer_detach (snd_mixer_t *mixer, const char *name)
      {
        static const char NAME[] = "snd_mixer_detach";
        typedef int ( *FunctionType)(snd_mixer_t *, const char *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer, name);
      }
      
      virtual int snd_mixer_load (snd_mixer_t *mixer)
      {
        static const char NAME[] = "snd_mixer_load";
        typedef int ( *FunctionType)(snd_mixer_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer);
      }
      
      virtual snd_mixer_elem_t * snd_mixer_elem_next (snd_mixer_elem_t *elem)
      {
        static const char NAME[] = "snd_mixer_elem_next";
        typedef snd_mixer_elem_t * ( *FunctionType)(snd_mixer_elem_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(elem);
      }
      
      virtual snd_mixer_elem_type_t snd_mixer_elem_get_type (const snd_mixer_elem_t *obj)
      {
        static const char NAME[] = "snd_mixer_elem_get_type";
        typedef snd_mixer_elem_type_t ( *FunctionType)(const snd_mixer_elem_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual const char * snd_mixer_selem_get_name (snd_mixer_elem_t *elem)
      {
        static const char NAME[] = "snd_mixer_selem_get_name";
        typedef const char * ( *FunctionType)(snd_mixer_elem_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(elem);
      }
      
      virtual int snd_mixer_selem_has_playback_volume (snd_mixer_elem_t *elem)
      {
        static const char NAME[] = "snd_mixer_selem_has_playback_volume";
        typedef int ( *FunctionType)(snd_mixer_elem_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(elem);
      }
      
      virtual int snd_mixer_selem_get_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)
      {
        static const char NAME[] = "snd_mixer_selem_get_playback_volume";
        typedef int ( *FunctionType)(snd_mixer_elem_t *, snd_mixer_selem_channel_id_t, long *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(elem, channel, value);
      }
      
      virtual int snd_mixer_selem_get_playback_volume_range (snd_mixer_elem_t *elem, long *min, long *max)
      {
        static const char NAME[] = "snd_mixer_selem_get_playback_volume_range";
        typedef int ( *FunctionType)(snd_mixer_elem_t *, long *, long *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(elem, min, max);
      }
      
      virtual int snd_mixer_selem_set_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value)
      {
        static const char NAME[] = "snd_mixer_selem_set_playback_volume";
        typedef int ( *FunctionType)(snd_mixer_elem_t *, snd_mixer_selem_channel_id_t, long);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(elem, channel, value);
      }
      
      virtual int snd_mixer_selem_register (snd_mixer_t *mixer, struct snd_mixer_selem_regopt *options, snd_mixer_class_t **classp)
      {
        static const char NAME[] = "snd_mixer_selem_register";
        typedef int ( *FunctionType)(snd_mixer_t *, struct snd_mixer_selem_regopt *, snd_mixer_class_t **);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mixer, options, classp);
      }
      
      virtual int snd_pcm_open (snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
      {
        static const char NAME[] = "snd_pcm_open";
        typedef int ( *FunctionType)(snd_pcm_t **, const char *, snd_pcm_stream_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm, name, stream, mode);
      }
      
      virtual int snd_pcm_hw_free (snd_pcm_t *pcm)
      {
        static const char NAME[] = "snd_pcm_hw_free";
        typedef int ( *FunctionType)(snd_pcm_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm);
      }
      
      virtual int snd_pcm_close (snd_pcm_t *pcm)
      {
        static const char NAME[] = "snd_pcm_close";
        typedef int ( *FunctionType)(snd_pcm_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm);
      }
      
      virtual int snd_pcm_prepare (snd_pcm_t *pcm)
      {
        static const char NAME[] = "snd_pcm_prepare";
        typedef int ( *FunctionType)(snd_pcm_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm);
      }
      
      virtual int snd_pcm_recover (snd_pcm_t *pcm, int err, int silent)
      {
        static const char NAME[] = "snd_pcm_recover";
        typedef int ( *FunctionType)(snd_pcm_t *, int, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm, err, silent);
      }
      
      virtual int snd_pcm_pause (snd_pcm_t *pcm, int enable)
      {
        static const char NAME[] = "snd_pcm_pause";
        typedef int ( *FunctionType)(snd_pcm_t *, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm, enable);
      }
      
      virtual int snd_pcm_drain (snd_pcm_t *pcm)
      {
        static const char NAME[] = "snd_pcm_drain";
        typedef int ( *FunctionType)(snd_pcm_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm);
      }
      
      virtual snd_pcm_sframes_t snd_pcm_writei (snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
      {
        static const char NAME[] = "snd_pcm_writei";
        typedef snd_pcm_sframes_t ( *FunctionType)(snd_pcm_t *, const void *, snd_pcm_uframes_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm, buffer, size);
      }
      
      virtual int snd_pcm_set_params (snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency)
      {
        static const char NAME[] = "snd_pcm_set_params";
        typedef int ( *FunctionType)(snd_pcm_t *, snd_pcm_format_t, snd_pcm_access_t, unsigned int, unsigned int, int, unsigned int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm, format, access, channels, rate, soft_resample, latency);
      }
      
      virtual int snd_pcm_format_mask_malloc (snd_pcm_format_mask_t ** ptr)
      {
        static const char NAME[] = "snd_pcm_format_mask_malloc";
        typedef int ( *FunctionType)(snd_pcm_format_mask_t **);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ptr);
      }
      
      virtual void snd_pcm_format_mask_free (snd_pcm_format_mask_t * obj)
      {
        static const char NAME[] = "snd_pcm_format_mask_free";
        typedef void ( *FunctionType)(snd_pcm_format_mask_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual int snd_pcm_format_mask_test (const snd_pcm_format_mask_t *mask, snd_pcm_format_t val)
      {
        static const char NAME[] = "snd_pcm_format_mask_test";
        typedef int ( *FunctionType)(const snd_pcm_format_mask_t *, snd_pcm_format_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(mask, val);
      }
      
      virtual int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t ** ptr)
      {
        static const char NAME[] = "snd_pcm_hw_params_malloc";
        typedef int ( *FunctionType)(snd_pcm_hw_params_t **);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ptr);
      }
      
      virtual void snd_pcm_hw_params_free (snd_pcm_hw_params_t * obj)
      {
        static const char NAME[] = "snd_pcm_hw_params_free";
        typedef void ( *FunctionType)(snd_pcm_hw_params_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual int snd_pcm_hw_params_any (snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
      {
        static const char NAME[] = "snd_pcm_hw_params_any";
        typedef int ( *FunctionType)(snd_pcm_t *, snd_pcm_hw_params_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcm, params);
      }
      
      virtual int snd_pcm_hw_params_can_pause (const snd_pcm_hw_params_t *params)
      {
        static const char NAME[] = "snd_pcm_hw_params_can_pause";
        typedef int ( *FunctionType)(const snd_pcm_hw_params_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(params);
      }
      
      virtual void snd_pcm_hw_params_get_format_mask (snd_pcm_hw_params_t *params, snd_pcm_format_mask_t *mask)
      {
        static const char NAME[] = "snd_pcm_hw_params_get_format_mask";
        typedef void ( *FunctionType)(snd_pcm_hw_params_t *, snd_pcm_format_mask_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(params, mask);
      }
      
      virtual int snd_pcm_info_malloc (snd_pcm_info_t ** ptr)
      {
        static const char NAME[] = "snd_pcm_info_malloc";
        typedef int ( *FunctionType)(snd_pcm_info_t **);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ptr);
      }
      
      virtual void snd_pcm_info_free (snd_pcm_info_t * obj)
      {
        static const char NAME[] = "snd_pcm_info_free";
        typedef void ( *FunctionType)(snd_pcm_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual void snd_pcm_info_set_device (snd_pcm_info_t *obj, unsigned int val)
      {
        static const char NAME[] = "snd_pcm_info_set_device";
        typedef void ( *FunctionType)(snd_pcm_info_t *, unsigned int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj, val);
      }
      
      virtual void snd_pcm_info_set_subdevice (snd_pcm_info_t *obj, unsigned int val)
      {
        static const char NAME[] = "snd_pcm_info_set_subdevice";
        typedef void ( *FunctionType)(snd_pcm_info_t *, unsigned int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj, val);
      }
      
      virtual void snd_pcm_info_set_stream (snd_pcm_info_t *obj, snd_pcm_stream_t val)
      {
        static const char NAME[] = "snd_pcm_info_set_stream";
        typedef void ( *FunctionType)(snd_pcm_info_t *, snd_pcm_stream_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj, val);
      }
      
      virtual int snd_card_next (int *card)
      {
        static const char NAME[] = "snd_card_next";
        typedef int ( *FunctionType)(int *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(card);
      }
      
      virtual int snd_ctl_open (snd_ctl_t **ctl, const char *name, int mode)
      {
        static const char NAME[] = "snd_ctl_open";
        typedef int ( *FunctionType)(snd_ctl_t **, const char *, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctl, name, mode);
      }
      
      virtual int snd_ctl_close (snd_ctl_t *ctl)
      {
        static const char NAME[] = "snd_ctl_close";
        typedef int ( *FunctionType)(snd_ctl_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctl);
      }
      
      virtual int snd_ctl_card_info (snd_ctl_t *ctl, snd_ctl_card_info_t *info)
      {
        static const char NAME[] = "snd_ctl_card_info";
        typedef int ( *FunctionType)(snd_ctl_t *, snd_ctl_card_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctl, info);
      }
      
      virtual int snd_ctl_card_info_malloc (snd_ctl_card_info_t ** ptr)
      {
        static const char NAME[] = "snd_ctl_card_info_malloc";
        typedef int ( *FunctionType)(snd_ctl_card_info_t **);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ptr);
      }
      
      virtual void snd_ctl_card_info_free (snd_ctl_card_info_t * obj)
      {
        static const char NAME[] = "snd_ctl_card_info_free";
        typedef void ( *FunctionType)(snd_ctl_card_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual const char * snd_ctl_card_info_get_id (const snd_ctl_card_info_t *obj)
      {
        static const char NAME[] = "snd_ctl_card_info_get_id";
        typedef const char * ( *FunctionType)(const snd_ctl_card_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual const char * snd_ctl_card_info_get_name (const snd_ctl_card_info_t *obj)
      {
        static const char NAME[] = "snd_ctl_card_info_get_name";
        typedef const char * ( *FunctionType)(const snd_ctl_card_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
      virtual int snd_ctl_pcm_next_device (snd_ctl_t * ctl, int * device)
      {
        static const char NAME[] = "snd_ctl_pcm_next_device";
        typedef int ( *FunctionType)(snd_ctl_t *, int *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctl, device);
      }
      
      virtual int snd_ctl_pcm_info (snd_ctl_t *ctl, snd_pcm_info_t *info)
      {
        static const char NAME[] = "snd_ctl_pcm_info";
        typedef int ( *FunctionType)(snd_ctl_t *, snd_pcm_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctl, info);
      }
      
      virtual const char * snd_pcm_info_get_name (const snd_pcm_info_t *obj)
      {
        static const char NAME[] = "snd_pcm_info_get_name";
        typedef const char * ( *FunctionType)(const snd_pcm_info_t *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(obj);
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      static const LibraryName NAME;
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(lib);
    }
  }
}
