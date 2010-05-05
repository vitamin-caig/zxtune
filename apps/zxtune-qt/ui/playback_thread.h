/*
Abstract:
  Playback thread interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYBACK_THREAD_H_DEFINED
#define ZXTUNE_QT_PLAYBACK_THREAD_H_DEFINED

//library includes
#include <core/module_types.h>
//qt includes
#include <QtCore/QThread>

class PlaybackThread : public QThread
{
  Q_OBJECT
public:
  static PlaybackThread* Create(QWidget* owner);

public slots:
  virtual void SetItem(const struct ModuleItem& item) = 0;
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

#endif //ZXTUNE_QT_PLAYBACK_THREAD_H_DEFINED
