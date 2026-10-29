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

#include <algorithm>
#include <vector>

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
      Chunks.clear();
      State = Devices::AYM::DataChunk();
      Blocked = false;
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

    bool SetValue(Devices::AYM::Stamp timeStamp, uint8_t val)
    {
      if (IsRegisterSelected())
      {
        const auto idx = static_cast<Devices::AYM::Registers::Index>(Register);
        if (Devices::AYM::DataChunk* chunk = GetChunk(timeStamp))
        {
          chunk->Data[idx] = val;
        }
        State.Data[idx] = val;
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

    Sound::Chunk RenderFrame(Devices::AYM::Stamp till)
    {
      AllocateChunk(till);
      // Writes carry the machine clock, which may run ahead of the output grid when a timer handler
      // overran the frame; keep those pending so the chip timeline stays on the grid
      // TODO: move this into the chip's own queue: RenderTill should slice the already rendered data
      // up to the requested time instead of letting RenderData advance the chip clock past it
      const auto split =
          std::stable_partition(Chunks.begin(), Chunks.end(),
                                [&till](const Devices::AYM::DataChunk& chunk) { return !(till < chunk.TimeStamp); });
      std::vector<Devices::AYM::DataChunk> pending(split, Chunks.end());
      Chunks.erase(split, Chunks.end());
      Chip->RenderData(Chunks);
      Chunks.swap(pending);
      return Chip->RenderTill(till);
    }

  private:
    bool IsRegisterSelected() const
    {
      return Register < Devices::AYM::Registers::TOTAL;
    }

    Devices::AYM::DataChunk* GetChunk(Devices::AYM::Stamp timeStamp)
    {
      return Blocked ? nullptr : &AllocateChunk(timeStamp);
    }

    Devices::AYM::DataChunk& AllocateChunk(Devices::AYM::Stamp timeStamp)
    {
      Chunks.resize(Chunks.size() + 1);
      Devices::AYM::DataChunk& res = Chunks.back();
      res.TimeStamp = timeStamp;
      return res;
    }

  private:
    const Devices::AYM::Chip::Ptr Chip;
    uint_t Register = 0;
    std::vector<Devices::AYM::DataChunk> Chunks;
    Devices::AYM::DataChunk State;
    bool Blocked = false;
  };
}  // namespace Module
