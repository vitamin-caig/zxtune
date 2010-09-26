/*
Abstract:
  DAC-based players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#ifndef __CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
#define __CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__

//library includes
#include <core/module_types.h>
#include <devices/dac.h>

namespace ZXTune
{
  namespace Module
  {
    class DACDevice : public Analyzer
    {
    public:
      typedef boost::shared_ptr<DACDevice> Ptr;

      //some virtuals from DAC::Device
      virtual void RenderData(const Sound::RenderParameters& params,
                              const DAC::DataChunk& src,
                              Sound::MultichannelReceiver& dst) = 0;

      virtual void Reset() = 0;

      //deprecated
      virtual void GetAnalyzer(Analyze::ChannelsState& analyzeState) const = 0;

      static Ptr Create(DAC::Chip::Ptr device);
    };
  }
}

#endif //__CORE_PLUGINS_PLAYERS_DAC_BASE_DEFINED__
