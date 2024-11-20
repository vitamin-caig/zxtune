/**
 *
 * @file
 *
 * @brief Information component interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <memory>

// forward declaration
namespace boost::program_options
{
  class options_description;
}  // namespace boost::program_options

class InformationComponent
{
public:
  virtual ~InformationComponent() = default;
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  // return true if should exit
  virtual bool Process(class SoundComponent& sound) const = 0;

  static std::unique_ptr<InformationComponent> Create();
};
