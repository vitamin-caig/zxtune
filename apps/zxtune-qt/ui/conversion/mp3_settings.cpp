/*
Abstract:
  MP3 settings widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "mp3_settings.h"
#include "mp3_settings.ui.h"
//common includes
#include <tools.h>

namespace
{
  const uint_t BITRATES[] =
  {
    16,
    32,
    64,
    96,
    112,
    128,
    160,
    192,
    256,
    320
  };

  class MP3Settings : public UI::MP3SettingsWidget
                    , private Ui::Mp3Settings
  {
  public:
    explicit MP3Settings(QWidget& parent)
      : UI::MP3SettingsWidget(parent)
    {
      //setup self
      setupUi(this);
      std::for_each(BITRATES, ArrayEnd(BITRATES), std::bind1st(std::mem_fun(&MP3Settings::AddBitrate), this));

      connect(selectDefault, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(selectCBR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(bitrateValue, SIGNAL(currentIndexChanged(int)), SIGNAL(SettingsChanged()));
      connect(selectVBR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(vbrQuality, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));
      connect(selectABR, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      connect(abrBitrate, SIGNAL(valueChanged(int)), SIGNAL(SettingsChanged()));

      selectDefault->setChecked(true);
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'m', 'p', '3', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      if (selectDefault->isChecked())
      {
        return selectDefault->text();
      }
      else if (selectCBR->isChecked())
      {
        return bitrateValue->currentText();
      }
      else if (selectVBR->isChecked())
      {
        return QString("VBR %1").arg(vbrQuality->value());
      }
      else if (selectABR->isChecked())
      {
        return QString("ABR %1 kbps").arg(abrBitrate->value());
      }
      else
      {
        return QString();
      }
    }
  private:
    void AddBitrate(uint_t brate) const
    {
      bitrateValue->addItem(QString("%1 kbps").arg(brate));
    }
  };
}

namespace UI
{
  MP3SettingsWidget::MP3SettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }

  MP3SettingsWidget* MP3SettingsWidget::Create(QWidget& parent)
  {
    return new MP3Settings(parent);
  }
}
