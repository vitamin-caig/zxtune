/*
Abstract:
  Playback support interface

Last changed:
  $Id: playback_thread.h 503 2010-05-16 11:08:27Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_PLAYBACK_SUPP_H_DEFINED
#define ZXTUNE_PLAYBACK_SUPP_H_DEFINED

//library includes
#include <core/module_types.h>
#include <sound/backend.h>
//qt includes
#include <QtCore/QThread>

class PlaybackSupport : public QThread
{
  Q_OBJECT
public:
  static PlaybackSupport* Create(QObject* owner);

public slots:
  virtual void SetItem(const class Playitem& item) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;
  virtual void Seek(int frame) = 0;
signals:
  void OnSetBackend(const ZXTune::Sound::Backend& backend);
  void OnStartModule(ZXTune::Module::Player::ConstPtr);
  void OnUpdateState();
  void OnPauseModule();
  void OnResumeModule();
  void OnStopModule();
  void OnFinishModule();
};

#endif //ZXTUNE_PLAYBACK_SUPP_H_DEFINED
