/*
Abstract:
  TFM-based stream players common functionality

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_BASE_STREAM_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_BASE_STREAM_DEFINED

//local includes
#include "tfm_chiptune.h"

namespace Module
{
  namespace TFM
  {
    class StreamModel
    {
    public:
      typedef boost::shared_ptr<const StreamModel> Ptr;
      virtual ~StreamModel() {}

      virtual uint_t Size() const = 0;
      virtual uint_t Loop() const = 0;
      virtual void Get(uint_t pos, Devices::TFM::Registers& res) const = 0;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties);
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_BASE_STREAM_DEFINED
