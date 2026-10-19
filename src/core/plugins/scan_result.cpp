/**
 *
 * @file
 *
 * @brief  Helper for scanning result implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/scan_result.h"

#include "core/plugins/parameters.h"

#include "parameters/accessor.h"

namespace ZXTune
{
  Analysis::Result::Ptr CreateUnmatchedResult(const Parameters::Accessor& params, Binary::Format::Ptr format,
                                              Binary::Container::Ptr data)
  {
    using namespace Parameters::ZXTune::Core::Plugins::Raw;
    if (const auto limit = Parameters::Get<ScanLimit>(params))
    {
      data = data->GetSubcontainer(0, limit);
    }
    return Analysis::CreateUnmatchedResult(std::move(format), std::move(data));
  }
}  // namespace ZXTune
