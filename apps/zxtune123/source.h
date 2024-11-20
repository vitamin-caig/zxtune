/**
 *
 * @file
 *
 * @brief Source data provider interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/data.h"
#include "module/holder.h"
#include "parameters/container.h"

#include "string_view.h"

#include <memory>
#include <stdexcept>

// forward declarations
namespace boost::program_options
{
  class options_description;
}  // namespace boost::program_options

class CancelError : public std::exception
{};

class OnItemCallback
{
public:
  virtual ~OnItemCallback() = default;

  virtual void ProcessItem(Binary::Data::Ptr data, Module::Holder::Ptr holder) = 0;
  virtual void ProcessUnknownData(StringView path, StringView container, Binary::Data::Ptr data){};
};

class SourceComponent
{
public:
  virtual ~SourceComponent() = default;
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  virtual void ParseParameters() = 0;
  // throw
  virtual void Initialize() = 0;
  virtual void ProcessItems(OnItemCallback& callback) = 0;

  static std::unique_ptr<SourceComponent> Create(Parameters::Container::Ptr configParams);
};
