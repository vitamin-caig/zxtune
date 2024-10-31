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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
