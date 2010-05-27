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
#include "status_control_ui.h"
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
  const QString EMPTY_TEXT(QString::fromUtf8("-"));

  class StatusControlImpl : public StatusControl
                          , private Ui::StatusControl
  {
  public:
    explicit StatusControlImpl(QWidget* parent)
    {
      setParent(parent);
      setupUi(this);
    }

    virtual void UpdateState(uint, const ZXTune::Module::Tracking& tracking)
    {
      textPosition->setText(QString::number(tracking.Position));
      textPattern->setText(QString::number(tracking.Pattern));
      textLine->setText(QString::number(tracking.Line));
      textChannels->setText(QString::number(tracking.Channels));
    }

    virtual void CloseState()
    {
      textPosition->setText(EMPTY_TEXT);
      textPattern->setText(EMPTY_TEXT);
      textLine->setText(EMPTY_TEXT);
      textChannels->setText(EMPTY_TEXT);
    }
  };
}

StatusControl* StatusControl::Create(QWidget* parent)
{
  assert(parent);
  return new StatusControlImpl(parent);
}
