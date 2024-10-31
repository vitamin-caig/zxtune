/**
 *
 * @file
 *
 * @brief Status control widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "sound/backend.h"

#include <QtWidgets/QWidget>

class PlaybackSupport;

class StatusControl : public QWidget
{
  Q_OBJECT
protected:
  explicit StatusControl(QWidget& parent);

public:
  // creator
  static StatusControl* Create(QWidget& parent, PlaybackSupport& supp);
};
