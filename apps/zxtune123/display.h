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

// library includes
#include <module/holder.h>
#include <sound/backend.h>
#include <strings/format.h>
#include <time/duration.h>
// std includes
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
  void Message(const char* msg, P&&... params)
  {
    Message(Strings::Format(msg, std::forward<P>(params)...));
  }

  virtual void Message(StringView msg) = 0;
  virtual void SetModule(Module::Holder::Ptr module, Sound::Backend::Ptr player) = 0;

  // begin frame, returns current position
  virtual Time::AtMillisecond BeginFrame(Sound::PlaybackControl::State state) = 0;

  static Ptr Create();
};
