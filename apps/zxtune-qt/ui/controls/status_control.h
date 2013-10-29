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

//library includes
#include <sound/backend.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class StatusControl : public QWidget
{
  Q_OBJECT
protected:
  explicit StatusControl(QWidget& parent);
public:
  //creator
  static StatusControl* Create(QWidget& parent, PlaybackSupport& supp);

public slots:
  virtual void InitState(Sound::Backend::Ptr) = 0;
  virtual void UpdateState() = 0;
  virtual void CloseState() = 0;
};
