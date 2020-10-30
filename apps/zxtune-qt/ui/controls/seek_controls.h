/**
* 
* @file
*
* @brief Seek controls widget interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "playlist/supp/data.h"
//library includes
#include <sound/backend.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class SeekControls : public QWidget
{
  Q_OBJECT
protected:
  explicit SeekControls(QWidget& parent);
public:
  //creator
  static SeekControls* Create(QWidget& parent, PlaybackSupport& supp);

public slots:
  virtual void InitState(Sound::Backend::Ptr, Playlist::Item::Data::Ptr) = 0;
  virtual void UpdateState() = 0;
  virtual void CloseState() = 0;
private slots:
  virtual void EndSeeking() = 0;
signals:
  void OnSeeking(Time::AtMillisecond);
};
