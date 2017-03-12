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

    std::vector<ChannelState> GetState() const override
    {
      const auto& in = Delegate->GetState();
      std::vector<ChannelState> out(in.size());
      std::transform(in.begin(), in.end(), out.begin(), &ConvertState);
      //required by compiler
      return std::move(out);
    }
  private:
    static ChannelState ConvertState(const Devices::ChannelState& in)
    {
      return {in.Band, in.Level.Raw()};
    }
  private:
    const Devices::StateSource::Ptr Delegate;
  };

  Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state)
  {
    return MakePtr<DevicesAnalyzer>(state);
  }
}
