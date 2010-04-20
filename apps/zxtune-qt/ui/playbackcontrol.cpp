/*
Abstract:
  Playback controller creation implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "../playbackcontrol.h"
#include "playback_controls.h"

namespace
{
  class PlaybackController : public QWidget, private Ui::PlaybackControls
  {
  public:
    PlaybackController()
    {
      setupUi(this);
    }
  };
};

namespace QtUi
{
  QPointer<QWidget> CreatePlaybackControlWidget()
  {
    return QPointer<QWidget>(new PlaybackController());
  }
}
