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

#include "apps/zxtune-qt/playlist/supp/data.h"

#include "sound/backend.h"

#include <QtWidgets/QWidget>

class PlaybackSupport;

class PlaybackOptions : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaybackOptions(QWidget& parent);

public:
  // creator
  static PlaybackOptions* Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params);
};
