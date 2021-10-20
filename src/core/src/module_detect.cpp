/**
 *
 * @file
 *
 * @brief  Module detection logic
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <core/module_detect.h>

namespace Module
{
  Holder::Ptr Open(const Parameters::Accessor& params, const Binary::Container& data,
                   Parameters::Container::Ptr initialProperties)
  {
    return {};
  }

  void Detect(const Parameters::Accessor& params, Binary::Container::Ptr data, DetectCallback& callback) {}

  std::size_t Detect(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location, DetectCallback& callback)
  {}
}  // namespace Module
