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

//library includes
#include <sound/backend.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class VolumeControl : public QWidget
{
  Q_OBJECT
protected:
  explicit VolumeControl(QWidget& parent);
public:
  //creator
  static VolumeControl* Create(QWidget& parent, PlaybackSupport& supp);

public slots:
  virtual void StartPlayback(Sound::Backend::Ptr) = 0;
  virtual void UpdateState() = 0;
  virtual void StopPlayback() = 0;
private slots:
  virtual void SetLevel(int level) = 0;
};
