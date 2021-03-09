/**
 *
 * @file
 *
 * @brief Playback options widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "playlist/supp/data.h"
// library includes
#include <sound/backend.h>
// qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class PlaybackOptions : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaybackOptions(QWidget& parent);

public:
  // creator
  static PlaybackOptions* Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params);
public slots:
  virtual void InitState(Sound::Backend::Ptr, Playlist::Item::Data::Ptr) = 0;
  virtual void UpdateState() = 0;
  virtual void CloseState() = 0;
};
