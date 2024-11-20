/**
 *
 * @file
 *
 * @brief Playback controls widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

class PlaybackSupport;

class PlaybackControls : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaybackControls(QWidget& parent);

public:
  // creator
  static PlaybackControls* Create(QWidget& parent, PlaybackSupport& supp);

  virtual class QMenu* GetActionsMenu() const = 0;
signals:
  void OnPlay();
  void OnPause();
  void OnStop();
  void OnPrevious();
  void OnNext();
};
