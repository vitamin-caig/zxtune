/*
Abstract:
  Playback support interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_PLAYBACK_SUPP_H_DEFINED
#define ZXTUNE_PLAYBACK_SUPP_H_DEFINED

//local includes
#include "playlist/supp/data.h"
//library includes
#include <core/module_types.h>
#include <sound/backend.h>
//qt includes
#include <QtCore/QThread>

class PlaybackSupport : public QObject
{
  Q_OBJECT
protected:
  explicit PlaybackSupport(QObject& parent);
public:
  static PlaybackSupport* Create(QObject& parent, Parameters::Accessor::Ptr sndOptions);

public slots:
  virtual void SetItem(Playlist::Item::Data::Ptr item) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;
  virtual void Seek(int frame) = 0;
signals:
  void OnSetBackend(ZXTune::Sound::Backend::Ptr);
  void OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr);
  void OnUpdateState();
  void OnPauseModule();
  void OnResumeModule();
  void OnStopModule();
  void OnFinishModule();
  void ErrorOccurred(const Error&);
};

ZXTune::Sound::CreateBackendParameters::Ptr CreateBackendParameters(Parameters::Accessor::Ptr params, ZXTune::Module::Holder::Ptr module, ZXTune::Sound::BackendCallback::Ptr callback);

#endif //ZXTUNE_PLAYBACK_SUPP_H_DEFINED
