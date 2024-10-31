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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
