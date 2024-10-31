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

#include "parameters/container.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
