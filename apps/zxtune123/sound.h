/**
 *
 * @file
 *
 * @brief Sound component interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/holder.h"
#include "parameters/container.h"
#include "sound/backend.h"
#include "time/duration.h"

#include "string_view.h"

#include <memory>
#include <span>

// forward declarations
namespace boost::program_options
{
  class options_description;
}  // namespace boost::program_options

class SoundComponent
{
public:
  virtual ~SoundComponent() = default;
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  virtual void ParseParameters() = 0;
  virtual void Initialize() = 0;
  // functional part
  virtual Sound::Backend::Ptr CreateBackend(Module::Holder::Ptr module, StringView typeHint = {},
                                            Sound::BackendCallback::Ptr callback = {}) = 0;

  virtual std::span<const Sound::BackendInformation::Ptr> EnumerateBackends() const = 0;

  virtual uint_t GetSamplerate() const = 0;

  static std::unique_ptr<SoundComponent> Create(Parameters::Container::Ptr configParams);
};
