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
//qt includes
#include <QtCore/QObject>

class PlaybackSupport : public QObject
{
  Q_OBJECT
public:
  static PlaybackSupport* Create(QWidget* owner);

public slots:
  virtual void SelectItem(const class Playitem& item) = 0;
  virtual void SetItem(const class Playitem& item) = 0;
  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;
  virtual void Seek(int frame) = 0;
signals:
  void OnStartModule(const ZXTune::Module::Information&);
  void OnUpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&);
  void OnPauseModule(const ZXTune::Module::Information&);
  void OnResumeModule(const ZXTune::Module::Information&);
  void OnStopModule(const ZXTune::Module::Information&);
  void OnFinishModule(const ZXTune::Module::Information&);
};

#endif //ZXTUNE_PLAYBACK_SUPP_H_DEFINED
