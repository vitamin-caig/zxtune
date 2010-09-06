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
#include "seek_controls_ui.h"
#include "seek_controls_moc.h"
#include "ui/utils.h"
//common includes
#include <formatter.h>

namespace
{
  class SeekControlsImpl : public SeekControls
                         , private Ui::SeekControls
  {
  public:
    explicit SeekControlsImpl(QWidget* parent)
    {
      setParent(parent);
      setupUi(this);
      timePosition->setRange(0, 0);
      this->connect(timePosition, SIGNAL(sliderMoved(int)), SIGNAL(OnSeeking(int)));
    }

    virtual void InitState(const ZXTune::Module::Information* info)
    {
      timePosition->setRange(0, info->FramesCount());
    }

    virtual void UpdateState(const ZXTune::Module::State& state)
    {
      if (!timePosition->isSliderDown())
      {
        timePosition->setValue(state.Frame);
      }
      timeDisplay->setText(ToQString(FormatTime(state.Frame, 20000/*TODO*/)));
    }

    virtual void CloseState()
    {
      timePosition->setRange(0, 0);
      timeDisplay->setText(QString::fromUtf8("-:-.-"));
    }
  };
}

SeekControls* SeekControls::Create(QWidget* parent)
{
  assert(parent);
  return new SeekControlsImpl(parent);
}
