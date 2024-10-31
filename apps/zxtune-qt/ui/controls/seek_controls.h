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

// local includes
#include "apps/zxtune-qt/playlist/supp/data.h"
// library includes
#include <sound/backend.h>
// qt includes
#include <QtWidgets/QWidget>

class PlaybackSupport;

class SeekControls : public QWidget
{
  Q_OBJECT
protected:
  explicit SeekControls(QWidget& parent);

public:
  // creator
  static SeekControls* Create(QWidget& parent, PlaybackSupport& supp);
signals:
  void OnSeeking(Time::AtMillisecond);
};
