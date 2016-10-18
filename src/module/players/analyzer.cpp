/**
* 
* @file
*
* @brief  Analyzer implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "analyzer.h"
//common includes
#include <make_ptr.h>
//std includes
#include <algorithm>
#include <utility>

namespace Module
{
  class DevicesAnalyzer : public Analyzer
  {
  public:
    explicit DevicesAnalyzer(Devices::StateSource::Ptr delegate)
      : Delegate(std::move(delegate))
    {
    }

    void GetState(std::vector<ChannelState>& channels) const override
    {
      Devices::MultiChannelState in;
      Delegate->GetState(in);
      channels.resize(in.size());
      std::transform(in.begin(), in.end(), channels.begin(), &ConvertState);
    }
  private:
    static ChannelState ConvertState(const Devices::ChannelState& in)
    {
      ChannelState out;
      out.Band = in.Band;
      out.Level = in.Level.Raw();
      return out;
    }
  private:
    const Devices::StateSource::Ptr Delegate;
  };

  Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state)
  {
    return MakePtr<DevicesAnalyzer>(state);
  }
}
