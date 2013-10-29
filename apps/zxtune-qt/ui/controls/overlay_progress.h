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

//qt includes
#include <QtGui/QWidget>

class OverlayProgress : public QWidget
{
  Q_OBJECT
protected:
  explicit OverlayProgress(QWidget& parent);
public:
  //creator
  static OverlayProgress* Create(QWidget& parent);
public slots:
  virtual void UpdateProgress(int progress) = 0;
signals:
  void Canceled();
};
