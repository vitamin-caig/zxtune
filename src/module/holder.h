/**
 *
 * @file
 *
 * @brief  Modules holder interface definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <module/information.h>
#include <module/renderer.h>
#include <parameters/accessor.h>

namespace Module
{
  //! @brief %Module holder interface
  class Holder
  {
  public:
    //! @brief Pointer type
    using Ptr = std::shared_ptr<const Holder>;

    virtual ~Holder() = default;

    //! @brief Retrieving info about loaded module
    virtual Information::Ptr GetModuleInformation() const = 0;

    //! @brief Retrieving properties of loaded module
    virtual Parameters::Accessor::Ptr GetModuleProperties() const = 0;

    //! @brief Creating new renderer instance
    //! @return New player
    virtual Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const = 0;
  };
}  // namespace Module
