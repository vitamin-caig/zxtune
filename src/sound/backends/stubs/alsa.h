const char * snd_asoundlib_version (void)
{
  return ASOUND_FUNC(snd_asoundlib_version)();
}

const char * snd_strerror (int errnum)
{
  return ASOUND_FUNC(snd_strerror)(errnum);
}

int snd_mixer_open (snd_mixer_t **mixer, int mode)
{
  return ASOUND_FUNC(snd_mixer_open)(mixer, mode);
}

int snd_mixer_close (snd_mixer_t *mixer)
{
  return ASOUND_FUNC(snd_mixer_close)(mixer);
}

snd_mixer_elem_t * snd_mixer_first_elem (snd_mixer_t *mixer)
{
  return ASOUND_FUNC(snd_mixer_first_elem)(mixer);
}

int snd_mixer_attach (snd_mixer_t *mixer, const char *name)
{
  return ASOUND_FUNC(snd_mixer_attach)(mixer, name);
}

int snd_mixer_detach (snd_mixer_t *mixer, const char *name)
{
  return ASOUND_FUNC(snd_mixer_detach)(mixer, name);
}

int snd_mixer_load (snd_mixer_t *mixer)
{
  return ASOUND_FUNC(snd_mixer_load)(mixer);
}

snd_mixer_elem_t * snd_mixer_elem_next (snd_mixer_elem_t *elem)
{
  return ASOUND_FUNC(snd_mixer_elem_next)(elem);
}

snd_mixer_elem_type_t snd_mixer_elem_get_type (const snd_mixer_elem_t *obj)
{
  return ASOUND_FUNC(snd_mixer_elem_get_type)(obj);
}

const char * snd_mixer_selem_get_name (snd_mixer_elem_t *elem)
{
  return ASOUND_FUNC(snd_mixer_selem_get_name)(elem);
}

int snd_mixer_selem_has_playback_volume (snd_mixer_elem_t *elem)
{
  return ASOUND_FUNC(snd_mixer_selem_has_playback_volume)(elem);
}

int snd_mixer_selem_get_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)
{
  return ASOUND_FUNC(snd_mixer_selem_get_playback_volume)(elem, channel, value);
}

int snd_mixer_selem_get_playback_volume_range (snd_mixer_elem_t *elem, long *min, long *max)
{
  return ASOUND_FUNC(snd_mixer_selem_get_playback_volume_range)(elem, min, max);
}

int snd_mixer_selem_set_playback_volume (snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value)
{
  return ASOUND_FUNC(snd_mixer_selem_set_playback_volume)(elem, channel, value);
}

int snd_mixer_selem_register (snd_mixer_t *mixer, struct snd_mixer_selem_regopt *options, snd_mixer_class_t **classp)
{
  return ASOUND_FUNC(snd_mixer_selem_register)(mixer, options, classp);
}

int snd_pcm_open (snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
{
  return ASOUND_FUNC(snd_pcm_open)(pcm, name, stream, mode);
}

int snd_pcm_close (snd_pcm_t *pcm)
{
  return ASOUND_FUNC(snd_pcm_close)(pcm);
}

int snd_pcm_prepare (snd_pcm_t *pcm)
{
  return ASOUND_FUNC(snd_pcm_prepare)(pcm);
}

int snd_pcm_pause (snd_pcm_t *pcm, int enable)
{
  return ASOUND_FUNC(snd_pcm_pause)(pcm, enable);
}

int snd_pcm_drain (snd_pcm_t *pcm)
{
  return ASOUND_FUNC(snd_pcm_drain)(pcm);
}

snd_pcm_sframes_t snd_pcm_writei (snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{
  return ASOUND_FUNC(snd_pcm_writei)(pcm, buffer, size);
}

size_t snd_pcm_format_mask_sizeof (void)
{
  return ASOUND_FUNC(snd_pcm_format_mask_sizeof)();
}

int snd_pcm_format_mask_test (const snd_pcm_format_mask_t *mask, snd_pcm_format_t val)
{
  return ASOUND_FUNC(snd_pcm_format_mask_test)(mask, val);
}

int snd_pcm_hw_params_any (snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
  return ASOUND_FUNC(snd_pcm_hw_params_any)(pcm, params);
}

int snd_pcm_hw_params_can_pause (const snd_pcm_hw_params_t *params)
{
  return ASOUND_FUNC(snd_pcm_hw_params_can_pause)(params);
}

void snd_pcm_hw_params_get_format_mask (snd_pcm_hw_params_t *params, snd_pcm_format_mask_t *mask)
{
  return ASOUND_FUNC(snd_pcm_hw_params_get_format_mask)(params, mask);
}

size_t snd_pcm_hw_params_sizeof (void)
{
  return ASOUND_FUNC(snd_pcm_hw_params_sizeof)();
}

int snd_pcm_hw_params (snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
  return ASOUND_FUNC(snd_pcm_hw_params)(pcm, params);
}

int snd_pcm_hw_free (snd_pcm_t *pcm)
{
  return ASOUND_FUNC(snd_pcm_hw_free)(pcm);
}

int snd_pcm_hw_params_set_access (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_access)(pcm, params, _access);
}

int snd_pcm_hw_params_set_format (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_format)(pcm, params, val);
}

int snd_pcm_hw_params_set_channels (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_channels)(pcm, params, val);
}

int snd_pcm_hw_params_set_rate_resample (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_rate_resample)(pcm, params, val);
}

int snd_pcm_hw_params_set_periods_near (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_periods_near)(pcm, params, val, dir);
}

int snd_pcm_hw_params_set_buffer_size_near (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_buffer_size_near)(pcm, params, val);
}

int snd_pcm_hw_params_set_rate (snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir)
{
  return ASOUND_FUNC(snd_pcm_hw_params_set_rate)(pcm, params, val, dir);
}

size_t snd_pcm_info_sizeof (void)
{
  return ASOUND_FUNC(snd_pcm_info_sizeof)();
}

void snd_pcm_info_set_device (snd_pcm_info_t *obj, unsigned int val)
{
  return ASOUND_FUNC(snd_pcm_info_set_device)(obj, val);
}

void snd_pcm_info_set_subdevice (snd_pcm_info_t *obj, unsigned int val)
{
  return ASOUND_FUNC(snd_pcm_info_set_subdevice)(obj, val);
}

void snd_pcm_info_set_stream (snd_pcm_info_t *obj, snd_pcm_stream_t val)
{
  return ASOUND_FUNC(snd_pcm_info_set_stream)(obj, val);
}

int snd_card_next (int *card)
{
  return ASOUND_FUNC(snd_card_next)(card);
}

int snd_ctl_open (snd_ctl_t **ctl, const char *name, int mode)
{
  return ASOUND_FUNC(snd_ctl_open)(ctl, name, mode);
}

int snd_ctl_close (snd_ctl_t *ctl)
{
  return ASOUND_FUNC(snd_ctl_close)(ctl);
}

int snd_ctl_card_info (snd_ctl_t *ctl, snd_ctl_card_info_t *info)
{
  return ASOUND_FUNC(snd_ctl_card_info)(ctl, info);
}

size_t snd_ctl_card_info_sizeof (void)
{
  return ASOUND_FUNC(snd_ctl_card_info_sizeof)();
}

const char * snd_ctl_card_info_get_id (const snd_ctl_card_info_t *obj)
{
  return ASOUND_FUNC(snd_ctl_card_info_get_id)(obj);
}

const char * snd_ctl_card_info_get_name (const snd_ctl_card_info_t *obj)
{
  return ASOUND_FUNC(snd_ctl_card_info_get_name)(obj);
}

int snd_ctl_pcm_next_device (snd_ctl_t * ctl, int * device)
{
  return ASOUND_FUNC(snd_ctl_pcm_next_device)(ctl, device);
}

int snd_ctl_pcm_info (snd_ctl_t *ctl, snd_pcm_info_t *info)
{
  return ASOUND_FUNC(snd_ctl_pcm_info)(ctl, info);
}

const char * snd_pcm_info_get_name (const snd_pcm_info_t *obj)
{
  return ASOUND_FUNC(snd_pcm_info_get_name)(obj);
}

