/**
* 
* @file
*
* @brief  Simple DAC-based tracks support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "dac_base.h"
#include "dac_properties_helper.h"
//library includes
#include <formats/chiptune/digital/digital.h>

namespace Module
{
  namespace DAC
  {
    class SimpleModuleData : public TrackModel
    {
    public:
      typedef std::shared_ptr<const SimpleModuleData> Ptr;
      typedef std::shared_ptr<SimpleModuleData> RWPtr;

      SimpleModuleData()
        : InitialTempo()
      {
      }

      uint_t GetInitialTempo() const override
      {
        return InitialTempo;
      }

      const OrderList& GetOrder() const override
      {
        return *Order;
      }

      const PatternsSet& GetPatterns() const override
      {
        return *Patterns;
      }

      uint_t InitialTempo;
      OrderList::Ptr Order;
      PatternsSet::Ptr Patterns;
      SparsedObjectsStorage<Devices::DAC::Sample::Ptr> Samples;
    };
    
    class SimpleDataBuilder : public Formats::Chiptune::Digital::Builder
    {
    public:
      typedef std::unique_ptr<SimpleDataBuilder> Ptr;
      
      virtual SimpleModuleData::Ptr GetResult() const = 0;
      
      static Ptr Create(DAC::PropertiesHelper& props, const PatternsBuilder& builder);
    };

    template<uint_t Channels>
    static SimpleDataBuilder::Ptr CreateSimpleDataBuilder(DAC::PropertiesHelper& props)
    {
      return SimpleDataBuilder::Create(props, PatternsBuilder::Create<Channels>());
    }
    
    DAC::Chiptune::Ptr CreateSimpleChiptune(SimpleModuleData::Ptr data, Parameters::Accessor::Ptr properties, uint_t channels);
  }
}
