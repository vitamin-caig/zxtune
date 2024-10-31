/**
 *
 * @file
 *
 * @brief Volume control widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <sound/backend.h>

#include <QtWidgets/QWidget>

class PlaybackSupport;

class VolumeControl : public QWidget
{
  Q_OBJECT
protected:
  explicit VolumeControl(QWidget& parent);

public:
  // creator
  static VolumeControl* Create(QWidget& parent, PlaybackSupport& supp);
};
