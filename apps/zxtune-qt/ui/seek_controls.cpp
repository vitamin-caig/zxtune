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
#include "utils.h"
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
      this->connect(timePosition, SIGNAL(valueChanged(int)), SIGNAL(OnSeeking(int)));
    }

    virtual void InitState(const ZXTune::Module::Information& info)
    {
      timePosition->setRange(0, info.Statistic.Frame);
    }

    virtual void UpdateState(uint frame, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)
    {
      timePosition->setValue(frame);
      timeDisplay->setText(ToQString(FormatTime(frame, 20000/*TODO*/)));
    }

    virtual void CloseState(const ZXTune::Module::Information&)
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
