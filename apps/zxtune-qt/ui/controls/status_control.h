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

// library includes
#include <sound/backend.h>
// qt includes
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
