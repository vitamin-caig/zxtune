/**
 *
 * @file
 *
 * @brief Overlay progress widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

class OverlayProgress : public QWidget
{
  Q_OBJECT
protected:
  explicit OverlayProgress(QWidget& parent);

public:
  // creator
  static OverlayProgress* Create(QWidget& parent);
  virtual void UpdateProgress(int progress) = 0;
signals:
  void Canceled();
};
