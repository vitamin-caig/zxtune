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

//library includes
#include <core/module_types.h>
#include <sound/backend.h>
//qt includes
#include <QtCore/QThread>

namespace Playlist
{
  namespace Item
  {
    class Data;
  }
}

class PlaybackSupport : public QThread
{
  Q_OBJECT
protected:
  explicit PlaybackSupport(QObject& parent);
public:
  static PlaybackSupport* Create(QObject& parent, Parameters::Accessor::Ptr sndOptions);

public slots:
  virtual void SetItem(const Playlist::Item::Data& item) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;
  virtual void Seek(int frame) = 0;
signals:
  void OnSetBackend(ZXTune::Sound::Backend::Ptr);
  void OnStartModule(ZXTune::Sound::Backend::Ptr);
  void OnUpdateState();
  void OnPauseModule();
  void OnResumeModule();
  void OnStopModule();
  void OnFinishModule();
};

#endif //ZXTUNE_PLAYBACK_SUPP_H_DEFINED
