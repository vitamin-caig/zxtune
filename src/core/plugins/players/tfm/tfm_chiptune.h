/*
Abstract:
  TFM-based chiptune interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_TFM_CHIPTUNE_DEFINED
#define CORE_PLUGINS_PLAYERS_TFM_CHIPTUNE_DEFINED

//local includes
#include "core/plugins/players/tracking.h"
//library includes
#include <devices/tfm.h>

namespace Module
{
  namespace TFM
  {
    class DataIterator : public StateIterator
    {
    public:
      typedef boost::shared_ptr<DataIterator> Ptr;

      virtual void GetData(Devices::TFM::Registers& res) const = 0;
    };

    class Chiptune
    {
    public:
      typedef boost::shared_ptr<const Chiptune> Ptr;
      virtual ~Chiptune() {}

      virtual Information::Ptr GetInformation() const = 0;
      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual DataIterator::Ptr CreateDataIterator() const = 0;
    };
  }
}

#endif //CORE_PLUGINS_PLAYERS_TFM_CHIPTUNE_DEFINED
