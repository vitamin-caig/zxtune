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

namespace
{
  class MixerWidgetImpl : public UI::MixerWidget
                        , private Ui::Mixer
  {
  public:
    explicit MixerWidgetImpl(QWidget& parent)
      : UI::MixerWidget(parent)
    {
      //setup self
      setupUi(this);

      connect(valueL, SIGNAL(valueChanged(int)), SIGNAL(ValueLeft(int)));
      connect(valueR, SIGNAL(valueChanged(int)), SIGNAL(ValueRight(int)));
    }
  };
}

namespace UI
{
  MixerWidget::MixerWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }
  
  MixerWidget* MixerWidget::Create(QWidget& parent)
  {
    return new MixerWidgetImpl(parent);
  }
}

namespace
{
  using namespace Parameters;
  
  const Char SUFFIX_L[] = {'.','l','e','f','t',0};
  const Char SUFFIX_R[] = {'.','r','i','g','h','t',0};

  class MixerValueImpl : public MixerValue
  {
  public:
    MixerValueImpl(UI::MixerWidget& parent, Parameters::Container& ctr, const Parameters::NameType& name, int defL, int defR)
      : MixerValue(parent)
      , Container(ctr)
      , NameL(name + SUFFIX_L)
      , NameR(name + SUFFIX_R)
    {
      Parameters::IntType valL = defL, valR = defR;
      Container.FindIntValue(NameL, valL);
      Container.FindIntValue(NameR, valR);
      connect(&parent, SIGNAL(ValueLeft(int)), SLOT(SetL(int)));
      connect(&parent, SIGNAL(ValueRight(int)), SLOT(SetR(int)));
    }

    virtual void SetL(int value)
    {
      Container.SetIntValue(NameL, value);
    }

    virtual void SetR(int value)
    {
      Container.SetIntValue(NameR, value);
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType NameL;
    const Parameters::NameType NameR;
  };
}

namespace Parameters
{
  MixerValue::MixerValue(QObject& parent) : QObject(&parent)
  {
  }

  void MixerValue::Bind(UI::MixerWidget& mix, Container& ctr, const NameType& name, int defL, int defR)
  {
    new MixerValueImpl(mix, ctr, name, defL, defR);
  }
}
