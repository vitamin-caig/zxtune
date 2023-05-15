/**
 *
 * @file
 *
 * @brief  ALSA subsystem API gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// platform-specific includes
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

namespace Sound
{
  namespace Alsa
  {
    class Api
    {
    public:
      typedef std::shared_ptr<Api> Ptr;
      virtual ~Api() = default;

// clang-format off

      virtual const char * snd_asoundlib_version (void) = 0;
      virtual const char * snd_strerror (int errnum) = 0;
      virtual int snd_config_update_free_global (void) = 0;
      virtual int snd_mixer_open (snd_mixer_t **mixer, int mode) = 0;
      virtual int snd_mixer_close (snd_mixer_t *mixer) = 0;
      virtual snd_mixer_elem_t * snd_mixer_first_elem (snd_mixer_t *mixer) = 0;
      virtual int snd_mixer_attach (snd_mixer_t *mixer, const char *name) = 0;
      virtual int snd_mixer_detach (snd_mixer_t *mixer, const char *name) = 0;
      virtual int snd_mixer_load (snd_mixer_t *mixer) = 0;
      virtual snd_mixer_elem_t * snd_mixer_elem_next (snd_mixer_elem_t *elem) = 0;
      virtual snd_mixer_elem_type_t snd_mixer_elem_get_type (const snd_mixer_elem_t *obj) = 0;
      virtual const char * snd_mixer_selem_get_name (snd_mixer_elem_t *elem) = 0;
      virtual int snd_mixer_selem_has_playback_volume (snd_mixer_elem_t *elem) = 0;
      virtual int snd_mixer_selem_get_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value) = 0;
      virtual int snd_mixer_selem_get_playback_volume_range (snd_mixer_elem_t *elem, long *min, long *max) = 0;
      virtual int snd_mixer_selem_set_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value) = 0;
      virtual int snd_mixer_selem_register (snd_mixer_t *mixer, struct snd_mixer_selem_regopt *options, snd_mixer_class_t **classp) = 0;
      virtual int snd_pcm_open (snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode) = 0;
      virtual int snd_pcm_hw_free (snd_pcm_t *pcm) = 0;
      virtual int snd_pcm_close (snd_pcm_t *pcm) = 0;
      virtual int snd_pcm_prepare (snd_pcm_t *pcm) = 0;
      virtual int snd_pcm_recover (snd_pcm_t *pcm, int err, int silent) = 0;
      virtual int snd_pcm_pause (snd_pcm_t *pcm, int enable) = 0;
      virtual int snd_pcm_drain (snd_pcm_t *pcm) = 0;
      virtual snd_pcm_sframes_t snd_pcm_writei (snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size) = 0;
      virtual int snd_pcm_set_params (snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency) = 0;
      virtual int snd_pcm_format_mask_malloc (snd_pcm_format_mask_t ** ptr) = 0;
      virtual void snd_pcm_format_mask_free (snd_pcm_format_mask_t * obj) = 0;
      virtual int snd_pcm_format_mask_test (const snd_pcm_format_mask_t *mask, snd_pcm_format_t val) = 0;
      virtual int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t ** ptr) = 0;
      virtual void snd_pcm_hw_params_free (snd_pcm_hw_params_t * obj) = 0;
      virtual int snd_pcm_hw_params_any (snd_pcm_t *pcm, snd_pcm_hw_params_t *params) = 0;
      virtual int snd_pcm_hw_params_can_pause (const snd_pcm_hw_params_t *params) = 0;
      virtual void snd_pcm_hw_params_get_format_mask (snd_pcm_hw_params_t *params, snd_pcm_format_mask_t *mask) = 0;
      virtual int snd_pcm_info_malloc (snd_pcm_info_t ** ptr) = 0;
      virtual void snd_pcm_info_free (snd_pcm_info_t * obj) = 0;
      virtual void snd_pcm_info_set_device (snd_pcm_info_t *obj, unsigned int val) = 0;
      virtual void snd_pcm_info_set_subdevice (snd_pcm_info_t *obj, unsigned int val) = 0;
      virtual void snd_pcm_info_set_stream (snd_pcm_info_t *obj, snd_pcm_stream_t val) = 0;
      virtual int snd_card_next (int *card) = 0;
      virtual int snd_ctl_open (snd_ctl_t **ctl, const char *name, int mode) = 0;
      virtual int snd_ctl_close (snd_ctl_t *ctl) = 0;
      virtual int snd_ctl_card_info (snd_ctl_t *ctl, snd_ctl_card_info_t *info) = 0;
      virtual int snd_ctl_card_info_malloc (snd_ctl_card_info_t ** ptr) = 0;
      virtual void snd_ctl_card_info_free (snd_ctl_card_info_t * obj) = 0;
      virtual const char * snd_ctl_card_info_get_id (const snd_ctl_card_info_t *obj) = 0;
      virtual const char * snd_ctl_card_info_get_name (const snd_ctl_card_info_t *obj) = 0;
      virtual int snd_ctl_pcm_next_device (snd_ctl_t * ctl, int * device) = 0;
      virtual int snd_ctl_pcm_info (snd_ctl_t *ctl, snd_pcm_info_t *info) = 0;
      virtual const char * snd_pcm_info_get_name (const snd_pcm_info_t *obj) = 0;
// clang-format on
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();
  }
}  // namespace Sound
