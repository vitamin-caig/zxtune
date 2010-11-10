/*
Abstract:
  Seek controller creation implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "seek_controls.h"
#include "seek_controls.ui.h"
#include "ui/utils.h"
//common includes
#include <formatter.h>

namespace
{
  class SeekControlsImpl : public SeekControls
                         , private Ui::SeekControls
  {
  public:
    explicit SeekControlsImpl(QWidget& parent)
      : ::SeekControls(parent)
    {
      //setup self
      setupUi(this);
      timePosition->setRange(0, 0);
      this->connect(timePosition, SIGNAL(sliderMoved(int)), SIGNAL(OnSeeking(int)));
    }

    virtual void InitState(ZXTune::Module::Player::ConstPtr player)
    {
      const ZXTune::Module::Information::Ptr info = player->GetInformation();
      timePosition->setRange(0, info->FramesCount());
      TrackState = player->GetTrackState();
    }

    virtual void UpdateState()
    {
      const uint_t curFrame = TrackState->Frame();
      if (!timePosition->isSliderDown())
      {
        timePosition->setValue(curFrame);
      }
      timeDisplay->setText(ToQString(FormatTime(curFrame, 20000/*TODO*/)));
    }

    virtual void CloseState()
    {
      timePosition->setRange(0, 0);
      timeDisplay->setText(QString::fromUtf8("-:-.-"));
    }
  private:
    ZXTune::Module::TrackState::Ptr TrackState;
  };
}

SeekControls::SeekControls(QWidget& parent) : QWidget(&parent)
{
}

SeekControls* SeekControls::Create(QWidget& parent)
{
  return new SeekControlsImpl(parent);
}
