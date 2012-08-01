/*
Abstract:
  Interactive mixer interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_PLAYBACK_MIXER_H_DEFINED
#define ZXTUNE_PLAYBACK_MIXER_H_DEFINED

//common includes
#include <parameters.h>
//library includes
#include <sound/mixer.h>
//qt includes
#include <QtCore/QObject>
#include <QtCore/QPointer>

class Mixer : public QObject
{
  Q_OBJECT
protected:
  explicit Mixer(QObject& parent);
public slots:
  virtual void Update() = 0;
};

Parameters::NameType GetMixerChannelParameterName(uint_t totalChannels, uint_t inChannel, uint_t outChannel);
int GetMixerChannelDefaultValue(uint_t totalChannels, uint_t inChannel, uint_t outChannel);
ZXTune::Sound::Mixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, uint_t channels);
ZXTune::Sound::Mixer::Ptr CreateMixer(class PlaybackSupport& supp, Parameters::Accessor::Ptr params, uint_t channels);

#endif //ZXTUNE_PLAYBACK_MIXER_H_DEFINED
