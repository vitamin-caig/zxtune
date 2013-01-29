/*
Abstract:
  TurboSound functionality helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__

//library includes
#include <core/module_holder.h>
#include <core/module_types.h>
#include <devices/aym/chip.h>
#include <devices/fm.h>

namespace ZXTune
{
  namespace Module
  {
    template<class Type>
    class DoubleReceiver : public DataReceiver<Type>
    {
    public:
      typedef boost::shared_ptr<DoubleReceiver<Type> > Ptr;

      virtual void SetStream(uint_t idx) = 0;
    };

    typedef DoubleReceiver<Devices::AYM::MultiSample> AYMTSMixer;
    typedef DoubleReceiver<Devices::FM::Sample> TFMMixer;

    TrackState::Ptr CreateTSTrackState(TrackState::Ptr first, TrackState::Ptr second);
    Analyzer::Ptr CreateTSAnalyzer(Analyzer::Ptr first, Analyzer::Ptr second);
    AYMTSMixer::Ptr CreateTSMixer(Devices::AYM::Receiver::Ptr delegate);
    TFMMixer::Ptr CreateTFMMixer(Devices::FM::Receiver::Ptr delegate);
    Renderer::Ptr CreateTSRenderer(Renderer::Ptr first, Renderer::Ptr second, AYMTSMixer::Ptr mixer);
  }
}
#endif //__CORE_PLUGINS_PLAYERS_TS_BASE_H_DEFINED__
