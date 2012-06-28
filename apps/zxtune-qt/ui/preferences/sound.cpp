/*
Abstract:
  Sound settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "sound.h"
#include "sound.ui.h"
#include "sound_alsa.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <sound/backend.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/algorithm/string/join.hpp>

namespace
{
  static const uint_t FREQUENCES[] =
  {
    8000,
    12000,
    16000,
    22000,
    24000,
    32000,
    44100,
    48000
  };

  StringArray GetSystemBackends(Parameters::Accessor::Ptr params)
  {
    StringArray result;
    const ZXTune::Sound::BackendsScope::Ptr scope = ZXTune::Sound::BackendsScope::CreateSystemScope(params);
    for (ZXTune::Sound::BackendCreator::Iterator::Ptr it = scope->Enumerate(); it->IsValid(); it->Next())
    {
      const ZXTune::Sound::BackendCreator::Ptr creator = it->Get();
      result.push_back(creator->Id());
    }
    return result;
  }

  class SoundOptionsWidget : public UI::SoundSettingsWidget
                           , public Ui::SoundOptions
  {
  public:
    explicit SoundOptionsWidget(QWidget& parent)
      : UI::SoundSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Backends(GetSystemBackends(Options))
    {
      //setup self
      setupUi(this);

      FillFrequences();
      FillBackends();

      Parameters::IntegerValue::Bind(*frameDurationValue, *Options, Parameters::ZXTune::Sound::FRAMEDURATION, Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT);
      Parameters::IntType freq = Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
      Options->FindValue(Parameters::ZXTune::Sound::FREQUENCY, freq);
      SetFrequency(freq);
      connect(soundFrequency, SIGNAL(currentIndexChanged(int)), SLOT(ChangeSoundFrequency(int)));

      connect(backendsList, SIGNAL(currentRowChanged(int)), SLOT(SelectBackend(int)));
      connect(moveUp, SIGNAL(released()), SLOT(MoveBackendUp()));
      connect(moveDown, SIGNAL(released()), SLOT(MoveBackendDown()));
    }

    virtual void ChangeSoundFrequency(int idx)
    {
      const qlonglong val = FREQUENCES[idx];
      Options->SetValue(Parameters::ZXTune::Sound::FREQUENCY, val);
    }
    
    virtual void SelectBackend(int idx)
    {
      const String id = Backends[idx];
      for (std::map<String, QWidget*>::const_iterator it = SetupPages.begin(), lim = SetupPages.end(); it != lim; ++it)
      {
        it->second->setVisible(it->first == id);
      }
    }
    
    virtual void MoveBackendUp()
    {
      if (int row = backendsList->currentRow())
      {
        SwapItems(row, row - 1);
        backendsList->setCurrentRow(row - 1);
      }
    }
    
    virtual void MoveBackendDown()
    {
      const int row = backendsList->currentRow();
      if (in_range<int>(row, 0, Backends.size() - 1))
      {
        SwapItems(row, row + 1);
        backendsList->setCurrentRow(row + 1);
      }
    }
  private:
    void FillFrequences()
    {
      std::for_each(FREQUENCES, ArrayEnd(FREQUENCES), boost::bind(&SoundOptionsWidget::AddFrequency, this, _1));
    }

    void AddFrequency(uint_t freq)
    {
      const QString txt = QString("%1 Hz").arg(freq);
      soundFrequency->addItem(txt);
    }

    void FillBackends()
    {
      std::for_each(Backends.begin(), Backends.end(), boost::bind(&SoundOptionsWidget::AddBackend, this, _1));
      AddPage(&UI::AlsaSettingsWidget::Create);
    }

    void AddPage(UI::BackendSettingsWidget* (*factory)(QWidget&))
    {
      std::auto_ptr<UI::BackendSettingsWidget> wid(factory(*backendSettings));
      const String id = wid->GetBackendId();
      if (Backends.end() != std::find(Backends.begin(), Backends.end(), id))
      {
        wid->hide();
        SetupPages[id] = wid.release();
      }
    }
    
    void AddBackend(const String& id)
    {
      backendsList->addItem(ToQString(id));
    }
    
    void SetFrequency(uint_t val)
    {
      const uint_t* const frq = std::find(FREQUENCES, ArrayEnd(FREQUENCES), val);
      if (frq != ArrayEnd(FREQUENCES))
      {
        soundFrequency->setCurrentIndex(frq - FREQUENCES);
      }
    }

    void SaveBackendsOrder()
    {
      static const Char DELIMITER[] = {';', 0};
      const String value = boost::algorithm::join(Backends, DELIMITER);
      Options->SetValue(Parameters::ZXTune::Sound::Backends::ORDER, value);
    }
    
    void SwapItems(int lh, int rh)
    {
      QListWidgetItem* const first = backendsList->item(lh);
      QListWidgetItem* const second = backendsList->item(rh);
      if (first && second)
      {
        const QString firstText = first->text();
        const QString secondText = second->text();
        String& firstId = Backends[lh];
        String& secondId = Backends[rh];
        assert(FromQString(firstText) == firstId);
        assert(FromQString(secondText) == secondId);
        first->setText(secondText);
        second->setText(firstText);
        std::swap(firstId, secondId);
        SaveBackendsOrder();
      }
    }
  private:
    const Parameters::Container::Ptr Options;
    StringArray Backends;
    std::map<String, QWidget*> SetupPages;
  };
}
namespace UI
{
  SoundSettingsWidget::SoundSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }

  SoundSettingsWidget* SoundSettingsWidget::Create(QWidget& parent)
  {
    return new SoundOptionsWidget(parent);
  }
}
