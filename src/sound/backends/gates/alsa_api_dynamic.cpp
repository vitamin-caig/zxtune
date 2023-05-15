/**
 *
 * @file
 *
 * @brief  ALSA subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/alsa_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace Alsa
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName() {}

      StringView Base() const override
      {
        return "asound"_sv;
      }

      std::vector<StringView> PosixAlternatives() const override
      {
        // deb-based + rpm-based
        return {"libasound.so.2.0.0"_sv, "libasound.so.2"_sv};
      }

      std::vector<StringView> WindowsAlternatives() const override
      {
        return {};
      }
    };

    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(std::move(lib))
      {
        Debug::Log("Sound::Backend::Alsa", "Library loaded");
      }

      ~DynamicApi() override
      {
        Debug::Log("Sound::Backend::Alsa", "Library unloaded");
      }

// clang-format off

      const char * snd_asoundlib_version (void) override
      {
        using FunctionType = decltype(&::snd_asoundlib_version);
        const auto func = Lib.GetSymbol<FunctionType>("snd_asoundlib_version");
        return func();
      }

      const char * snd_strerror (int errnum) override
      {
        using FunctionType = decltype(&::snd_strerror);
        const auto func = Lib.GetSymbol<FunctionType>("snd_strerror");
        return func(errnum);
      }

      int snd_config_update_free_global (void) override
      {
        using FunctionType = decltype(&::snd_config_update_free_global);
        const auto func = Lib.GetSymbol<FunctionType>("snd_config_update_free_global");
        return func();
      }

      int snd_mixer_open (snd_mixer_t **mixer, int mode) override
      {
        using FunctionType = decltype(&::snd_mixer_open);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_open");
        return func(mixer, mode);
      }

      int snd_mixer_close (snd_mixer_t *mixer) override
      {
        using FunctionType = decltype(&::snd_mixer_close);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_close");
        return func(mixer);
      }

      snd_mixer_elem_t * snd_mixer_first_elem (snd_mixer_t *mixer) override
      {
        using FunctionType = decltype(&::snd_mixer_first_elem);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_first_elem");
        return func(mixer);
      }

      int snd_mixer_attach (snd_mixer_t *mixer, const char *name) override
      {
        using FunctionType = decltype(&::snd_mixer_attach);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_attach");
        return func(mixer, name);
      }

      int snd_mixer_detach (snd_mixer_t *mixer, const char *name) override
      {
        using FunctionType = decltype(&::snd_mixer_detach);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_detach");
        return func(mixer, name);
      }

      int snd_mixer_load (snd_mixer_t *mixer) override
      {
        using FunctionType = decltype(&::snd_mixer_load);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_load");
        return func(mixer);
      }

      snd_mixer_elem_t * snd_mixer_elem_next (snd_mixer_elem_t *elem) override
      {
        using FunctionType = decltype(&::snd_mixer_elem_next);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_elem_next");
        return func(elem);
      }

      snd_mixer_elem_type_t snd_mixer_elem_get_type (const snd_mixer_elem_t *obj) override
      {
        using FunctionType = decltype(&::snd_mixer_elem_get_type);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_elem_get_type");
        return func(obj);
      }

      const char * snd_mixer_selem_get_name (snd_mixer_elem_t *elem) override
      {
        using FunctionType = decltype(&::snd_mixer_selem_get_name);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_selem_get_name");
        return func(elem);
      }

      int snd_mixer_selem_has_playback_volume (snd_mixer_elem_t *elem) override
      {
        using FunctionType = decltype(&::snd_mixer_selem_has_playback_volume);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_selem_has_playback_volume");
        return func(elem);
      }

      int snd_mixer_selem_get_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) override
      {
        using FunctionType = decltype(&::snd_mixer_selem_get_playback_volume);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_selem_get_playback_volume");
        return func(elem, channel, value);
      }

      int snd_mixer_selem_get_playback_volume_range (snd_mixer_elem_t *elem, long *min, long *max) override
      {
        using FunctionType = decltype(&::snd_mixer_selem_get_playback_volume_range);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_selem_get_playback_volume_range");
        return func(elem, min, max);
      }

      int snd_mixer_selem_set_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value) override
      {
        using FunctionType = decltype(&::snd_mixer_selem_set_playback_volume);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_selem_set_playback_volume");
        return func(elem, channel, value);
      }

      int snd_mixer_selem_register (snd_mixer_t *mixer, struct snd_mixer_selem_regopt *options, snd_mixer_class_t **classp) override
      {
        using FunctionType = decltype(&::snd_mixer_selem_register);
        const auto func = Lib.GetSymbol<FunctionType>("snd_mixer_selem_register");
        return func(mixer, options, classp);
      }

      int snd_pcm_open (snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode) override
      {
        using FunctionType = decltype(&::snd_pcm_open);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_open");
        return func(pcm, name, stream, mode);
      }

      int snd_pcm_hw_free (snd_pcm_t *pcm) override
      {
        using FunctionType = decltype(&::snd_pcm_hw_free);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_hw_free");
        return func(pcm);
      }

      int snd_pcm_close (snd_pcm_t *pcm) override
      {
        using FunctionType = decltype(&::snd_pcm_close);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_close");
        return func(pcm);
      }

      int snd_pcm_prepare (snd_pcm_t *pcm) override
      {
        using FunctionType = decltype(&::snd_pcm_prepare);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_prepare");
        return func(pcm);
      }

      int snd_pcm_recover (snd_pcm_t *pcm, int err, int silent) override
      {
        using FunctionType = decltype(&::snd_pcm_recover);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_recover");
        return func(pcm, err, silent);
      }

      int snd_pcm_pause (snd_pcm_t *pcm, int enable) override
      {
        using FunctionType = decltype(&::snd_pcm_pause);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_pause");
        return func(pcm, enable);
      }

      int snd_pcm_drain (snd_pcm_t *pcm) override
      {
        using FunctionType = decltype(&::snd_pcm_drain);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_drain");
        return func(pcm);
      }

      snd_pcm_sframes_t snd_pcm_writei (snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size) override
      {
        using FunctionType = decltype(&::snd_pcm_writei);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_writei");
        return func(pcm, buffer, size);
      }

      int snd_pcm_set_params (snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency) override
      {
        using FunctionType = decltype(&::snd_pcm_set_params);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_set_params");
        return func(pcm, format, access, channels, rate, soft_resample, latency);
      }

      int snd_pcm_format_mask_malloc (snd_pcm_format_mask_t ** ptr) override
      {
        using FunctionType = decltype(&::snd_pcm_format_mask_malloc);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_format_mask_malloc");
        return func(ptr);
      }

      void snd_pcm_format_mask_free (snd_pcm_format_mask_t * obj) override
      {
        using FunctionType = decltype(&::snd_pcm_format_mask_free);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_format_mask_free");
        return func(obj);
      }

      int snd_pcm_format_mask_test (const snd_pcm_format_mask_t *mask, snd_pcm_format_t val) override
      {
        using FunctionType = decltype(&::snd_pcm_format_mask_test);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_format_mask_test");
        return func(mask, val);
      }

      int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t ** ptr) override
      {
        using FunctionType = decltype(&::snd_pcm_hw_params_malloc);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_hw_params_malloc");
        return func(ptr);
      }

      void snd_pcm_hw_params_free (snd_pcm_hw_params_t * obj) override
      {
        using FunctionType = decltype(&::snd_pcm_hw_params_free);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_hw_params_free");
        return func(obj);
      }

      int snd_pcm_hw_params_any (snd_pcm_t *pcm, snd_pcm_hw_params_t *params) override
      {
        using FunctionType = decltype(&::snd_pcm_hw_params_any);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_hw_params_any");
        return func(pcm, params);
      }

      int snd_pcm_hw_params_can_pause (const snd_pcm_hw_params_t *params) override
      {
        using FunctionType = decltype(&::snd_pcm_hw_params_can_pause);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_hw_params_can_pause");
        return func(params);
      }

      void snd_pcm_hw_params_get_format_mask (snd_pcm_hw_params_t *params, snd_pcm_format_mask_t *mask) override
      {
        using FunctionType = decltype(&::snd_pcm_hw_params_get_format_mask);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_hw_params_get_format_mask");
        return func(params, mask);
      }

      int snd_pcm_info_malloc (snd_pcm_info_t ** ptr) override
      {
        using FunctionType = decltype(&::snd_pcm_info_malloc);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_info_malloc");
        return func(ptr);
      }

      void snd_pcm_info_free (snd_pcm_info_t * obj) override
      {
        using FunctionType = decltype(&::snd_pcm_info_free);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_info_free");
        return func(obj);
      }

      void snd_pcm_info_set_device (snd_pcm_info_t *obj, unsigned int val) override
      {
        using FunctionType = decltype(&::snd_pcm_info_set_device);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_info_set_device");
        return func(obj, val);
      }

      void snd_pcm_info_set_subdevice (snd_pcm_info_t *obj, unsigned int val) override
      {
        using FunctionType = decltype(&::snd_pcm_info_set_subdevice);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_info_set_subdevice");
        return func(obj, val);
      }

      void snd_pcm_info_set_stream (snd_pcm_info_t *obj, snd_pcm_stream_t val) override
      {
        using FunctionType = decltype(&::snd_pcm_info_set_stream);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_info_set_stream");
        return func(obj, val);
      }

      int snd_card_next (int *card) override
      {
        using FunctionType = decltype(&::snd_card_next);
        const auto func = Lib.GetSymbol<FunctionType>("snd_card_next");
        return func(card);
      }

      int snd_ctl_open (snd_ctl_t **ctl, const char *name, int mode) override
      {
        using FunctionType = decltype(&::snd_ctl_open);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_open");
        return func(ctl, name, mode);
      }

      int snd_ctl_close (snd_ctl_t *ctl) override
      {
        using FunctionType = decltype(&::snd_ctl_close);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_close");
        return func(ctl);
      }

      int snd_ctl_card_info (snd_ctl_t *ctl, snd_ctl_card_info_t *info) override
      {
        using FunctionType = decltype(&::snd_ctl_card_info);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_card_info");
        return func(ctl, info);
      }

      int snd_ctl_card_info_malloc (snd_ctl_card_info_t ** ptr) override
      {
        using FunctionType = decltype(&::snd_ctl_card_info_malloc);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_card_info_malloc");
        return func(ptr);
      }

      void snd_ctl_card_info_free (snd_ctl_card_info_t * obj) override
      {
        using FunctionType = decltype(&::snd_ctl_card_info_free);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_card_info_free");
        return func(obj);
      }

      const char * snd_ctl_card_info_get_id (const snd_ctl_card_info_t *obj) override
      {
        using FunctionType = decltype(&::snd_ctl_card_info_get_id);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_card_info_get_id");
        return func(obj);
      }

      const char * snd_ctl_card_info_get_name (const snd_ctl_card_info_t *obj) override
      {
        using FunctionType = decltype(&::snd_ctl_card_info_get_name);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_card_info_get_name");
        return func(obj);
      }

      int snd_ctl_pcm_next_device (snd_ctl_t * ctl, int * device) override
      {
        using FunctionType = decltype(&::snd_ctl_pcm_next_device);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_pcm_next_device");
        return func(ctl, device);
      }

      int snd_ctl_pcm_info (snd_ctl_t *ctl, snd_pcm_info_t *info) override
      {
        using FunctionType = decltype(&::snd_ctl_pcm_info);
        const auto func = Lib.GetSymbol<FunctionType>("snd_ctl_pcm_info");
        return func(ctl, info);
      }

      const char * snd_pcm_info_get_name (const snd_pcm_info_t *obj) override
      {
        using FunctionType = decltype(&::snd_pcm_info_get_name);
        const auto func = Lib.GetSymbol<FunctionType>("snd_pcm_info_get_name");
        return func(obj);
      }

// clang-format on
    private:
      const Platform::SharedLibraryAdapter Lib;
    };

    Api::Ptr LoadDynamicApi()
    {
      static const LibraryName NAME;
      auto lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace Alsa
}  // namespace Sound
