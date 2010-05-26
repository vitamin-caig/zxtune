/*
Abstract:
  Status control widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "status_control.h"
#include "status_control_moc.h"
#include "ui/utils.h"
//common includes
#include <parameters.h>
//library includes
#include <core/module_attrs.h>
//qt includes
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

namespace
{
  class StatusControlImpl : public StatusControl
  {
  public:
    explicit StatusControlImpl(QWidget* parent)
      : TitleLabel(new QLabel(this))
    {
      setParent(parent);
      QGridLayout* const layout = new QGridLayout(this);
      layout->setSpacing(1);
      layout->setMargin(2);

      layout->addWidget(TitleLabel, 0, 0);
    }

    virtual void InitState(const ZXTune::Module::Information& info)
    {
      String result, title;
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_AUTHOR, result);
      Parameters::FindByName(info.Properties, ZXTune::Module::ATTR_TITLE, title);
      if (!result.empty())
      {
        result += ' ';
        result += '-';
        result += ' ';
      }
      result += title;
      TitleLabel->setText(ToQString(result));
    }

    virtual void UpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&)
    {
    }

    virtual void CloseState()
    {
      TitleLabel->clear();
    }
  private:
    QLabel* const TitleLabel;
  };
}

StatusControl* StatusControl::Create(QWidget* parent)
{
  assert(parent);
  return new StatusControlImpl(parent);
}
