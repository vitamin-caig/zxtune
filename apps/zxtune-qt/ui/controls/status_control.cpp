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
#include "status_control.ui.h"
#include "supp/playback_supp.h"
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
  const QString EMPTY_TEXT(QLatin1String("-"));

  class StatusControlImpl : public StatusControl
                          , private Ui::StatusControl
  {
  public:
    StatusControlImpl(QWidget& parent, PlaybackSupport& supp)
      : ::StatusControl(parent)
    {
      //setup self
      setupUi(this);

      this->connect(&supp, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(InitState(ZXTune::Sound::Backend::Ptr)));
      this->connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState()));
      this->connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState()));
    }

    virtual void InitState(ZXTune::Sound::Backend::Ptr player)
    {
      TrackState = player->GetTrackState();
      CloseState();
    }

    virtual void UpdateState()
    {
      textPosition->setText(QString::number(TrackState->Position()));
      textPattern->setText(QString::number(TrackState->Pattern()));
      textLine->setText(QString::number(TrackState->Line()));
      textFrame->setText(QString::number(TrackState->Quirk()));
      textChannels->setText(QString::number(TrackState->Channels()));
      textTempo->setText(QString::number(TrackState->Tempo()));
    }

    virtual void CloseState()
    {
      textPosition->setText(EMPTY_TEXT);
      textPattern->setText(EMPTY_TEXT);
      textLine->setText(EMPTY_TEXT);
      textFrame->setText(EMPTY_TEXT);
      textChannels->setText(EMPTY_TEXT);
      textTempo->setText(EMPTY_TEXT);
    }
  private:
    ZXTune::Module::TrackState::Ptr TrackState;
  };
}

StatusControl::StatusControl(QWidget& parent) : QWidget(&parent)
{
}

StatusControl* StatusControl::Create(QWidget& parent, PlaybackSupport& supp)
{
  return new StatusControlImpl(parent, supp);
}
