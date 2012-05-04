/*
Abstract:
  Single channel mixer widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "mixer.h"
#include "mixer.ui.h"
//common includes
#include <contract.h>

namespace
{
  class MixerWidgetImpl : public UI::MixerWidget
                        , private Ui::Mixer
  {
  public:
    MixerWidgetImpl(QWidget& parent, const QString& name)
      : UI::MixerWidget(parent)
    {
      //setup self
      setupUi(this);
      channelName->setText(name);

      Require(connect(channelValue, SIGNAL(valueChanged(int)), SIGNAL(valueChanged(int))));
    }

    virtual void setValue(int val)
    {
      channelValue->setValue(val);
    }
  };
}

namespace UI
{
  MixerWidget::MixerWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }
  
  MixerWidget* MixerWidget::Create(QWidget& parent, const QString& name)
  {
    return new MixerWidgetImpl(parent, name);
  }
}

namespace
{
  using namespace Parameters;
  
  class MixerValueImpl : public MixerValue
  {
  public:
    MixerValueImpl(UI::MixerWidget& parent, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
      : MixerValue(parent)
      , Container(ctr)
      , Name(name)
    {
      Parameters::IntType value = defValue;
      Container.FindValue(Name, value);
      parent.setValue(value);
      Require(connect(&parent, SIGNAL(valueChanged(int)), SLOT(SetValue(int))));
    }

    virtual void SetValue(int value)
    {
      Container.SetValue(Name, value);
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
  };
}

namespace Parameters
{
  MixerValue::MixerValue(QObject& parent) : QObject(&parent)
  {
  }

  void MixerValue::Bind(UI::MixerWidget& mix, Container& ctr, const NameType& name, int defValue)
  {
    new MixerValueImpl(mix, ctr, name, defValue);
  }
}
