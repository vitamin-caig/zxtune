/**
 *
 * @file
 *
 * @brief Display component interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/holder.h"
#include "sound/backend.h"
#include "strings/format.h"
#include "time/duration.h"

#include "string_view.h"

#include <memory>

// forward declarations
namespace boost::program_options
{
  class options_description;
}  // namespace boost::program_options

class DisplayComponent
{
public:
  using Ptr = std::unique_ptr<DisplayComponent>;

  virtual ~DisplayComponent() = default;

  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;

  template<class... P>
  void Message(Strings::FormatString<P...> msg, P&&... params)
  {
    Message(Strings::Format(msg, std::forward<P>(params)...));
  }

  virtual void Message(StringView msg) = 0;
  virtual void SetModule(const Module::Holder& module, const Sound::Backend& player) = 0;

  virtual void BeginFrame(Sound::PlaybackControl::PlaybackState playbackState, const Module::State& moduleState) = 0;

  static Ptr Create();
};
