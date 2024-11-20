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

#include "time/instant.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
