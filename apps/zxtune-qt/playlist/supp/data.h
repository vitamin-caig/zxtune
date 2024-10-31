/**
 *
 * @file
 *
 * @brief Playlist entity interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "apps/zxtune-qt/playlist/supp/capabilities.h"
// common includes
#include <error.h>
// library includes
#include <binary/data.h>
#include <module/holder.h>
#include <parameters/container.h>
#include <time/duration.h>
#include <tools/iterators.h>
// std includes
#include <variant>

namespace Playlist::Item
{
  class ModuleState
  {
  public:
    static ModuleState Make();
    static ModuleState MakeLoading();
    static ModuleState MakeReady(Error err = Error());

    ModuleState& operator=(const ModuleState&) = default;

    bool IsLoading() const;
    bool IsReady() const;
    const Error* GetIfError() const;

  private:
    template<class T>
    explicit ModuleState(T&& val)
      : Container(std::move(val))
    {}

  private:
    struct Empty
    {};
    struct Loading
    {};
    struct Ready
    {};
    std::variant<Empty, Loading, Ready, Error> Container;
  };

  class Data
  {
  public:
    using Ptr = std::shared_ptr<const Data>;

    virtual ~Data() = default;

    // common
    //  eager objects
    virtual Module::Holder::Ptr GetModule() const = 0;
    virtual Binary::Data::Ptr GetModuleData() const = 0;
    //  lightweight objects
    virtual Parameters::Accessor::Ptr GetModuleProperties() const = 0;
    virtual Parameters::Container::Ptr GetAdjustedParameters() const = 0;
    virtual Capabilities GetCapabilities() const = 0;
    // playlist-related
    virtual ModuleState GetState() const = 0;
    virtual String GetFullPath() const = 0;
    virtual String GetFilePath() const = 0;
    virtual String GetType() const = 0;
    virtual String GetDisplayName() const = 0;
    virtual Time::Milliseconds GetDuration() const = 0;
    virtual String GetAuthor() const = 0;
    virtual String GetTitle() const = 0;
    virtual String GetComment() const = 0;
    virtual uint32_t GetChecksum() const = 0;
    virtual uint32_t GetCoreChecksum() const = 0;
    virtual std::size_t GetSize() const = 0;
  };

  using Collection = ObjectIterator<Data::Ptr>;
}  // namespace Playlist::Item
