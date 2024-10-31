/**
 *
 * @file
 *
 * @brief  AY/YM dump builder interface and factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/aym/dumper.h"

namespace Devices::AYM
{
  class DumpBuilder
  {
  public:
    virtual ~DumpBuilder() = default;

    virtual void Initialize() = 0;

    virtual Binary::Data::Ptr GetResult() = 0;
  };

  class FramedDumpBuilder : public DumpBuilder
  {
  public:
    using Ptr = std::unique_ptr<FramedDumpBuilder>;

    virtual void WriteFrame(uint_t framesPassed, const Registers& state, const Registers& update) = 0;
  };

  Dumper::Ptr CreateDumper(const DumperParameters& params, FramedDumpBuilder::Ptr builder);

  // internal factories
  FramedDumpBuilder::Ptr CreateRawDumpBuilder();
}  // namespace Devices::AYM
