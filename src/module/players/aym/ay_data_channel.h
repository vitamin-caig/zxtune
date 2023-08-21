/**
 *
 * @file
 *
 * @brief  AY data channel adapter for emulation-based formats
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "devices/aym/chip.h"

#include <optional>
#include <utility>

namespace Module
{
  class AyDataChannel
  {
  public:
    explicit AyDataChannel(Devices::AYM::Chip::Ptr chip)
      : Chip(std::move(chip))
    {}

    void Reset()
    {
      Chip->Reset();
      Register = 0;
      State = Devices::AYM::DataChunk();
      Pending.reset();
      Blocked = false;
      FirstWrite.reset();
    }

    //! Timestamp of the first queued AY write; used to define the track timeline start
    std::optional<Devices::AYM::Stamp> FirstSoundWrite() const
    {
      return FirstWrite;
    }

    void SetBlocked(bool block)
    {
      Blocked = block;
    }

    bool SelectRegister(uint_t reg)
    {
      Register = reg;
      return IsRegisterSelected();
    }

    bool SetValue(const Devices::AYM::Stamp& timeStamp, uint8_t val)
    {
      if (IsRegisterSelected())
      {
        const auto idx = static_cast<Devices::AYM::Registers::Index>(Register);
        State.Data[idx] = val;
        if (!Blocked)
        {
          if (!FirstWrite)
          {
            FirstWrite = timeStamp;
          }
          // The chip requires strictly increasing chunk timestamps, so writes made at the same
          // (microsecond) moment are grouped into a single chunk
          if (!Pending || Pending->TimeStamp.Get() != timeStamp.Get())
          {
            FlushPending();
            Pending.emplace();
            Pending->TimeStamp = timeStamp;
          }
          Pending->Data[idx] = val;
        }
        return true;
      }
      return false;
    }

    uint8_t GetValue() const
    {
      if (IsRegisterSelected())
      {
        return State.Data[static_cast<Devices::AYM::Registers::Index>(Register)];
      }
      else
      {
        return 0xff;
      }
    }

    Sound::Chunk RenderTill(const Devices::AYM::Stamp& till)
    {
      FlushPending();
      return Chip->RenderTill(till);
    }

  private:
    bool IsRegisterSelected() const
    {
      return Register < Devices::AYM::Registers::TOTAL;
    }

    void FlushPending()
    {
      if (Pending)
      {
        Chip->RenderData(std::move(*Pending));
        Pending.reset();
      }
    }

  private:
    const Devices::AYM::Chip::Ptr Chip;
    uint_t Register = 0;
    Devices::AYM::DataChunk State;
    std::optional<Devices::AYM::DataChunk> Pending;
    bool Blocked = false;
    std::optional<Devices::AYM::Stamp> FirstWrite;
  };
}  // namespace Module
