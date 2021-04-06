/**
 *
 * @file
 *
 * @brief AYL format tags
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include <types.h>

namespace AYL
{
  constexpr const auto SUFFIX = ".ayl"_sv;

  constexpr const auto SIGNATURE = "ZX Spectrum Sound Chip Emulator Play List File v1."_sv;

  constexpr const auto CHIP_FREQUENCY = "ChipFrequency"_sv;

  constexpr const auto PLAYER_FREQUENCY = "PlayerFrequency"_sv;

  constexpr const auto CHIP_TYPE = "ChipType"_sv;

  constexpr const auto CHANNELS_ALLOCATION = "ChannelsAllocation"_sv;

  constexpr const auto NAME = "Name"_sv;

  constexpr const auto AUTHOR = "Author"_sv;

  constexpr const auto PROGRAM = "Program"_sv;

  constexpr const auto TRACKER = "Tracker"_sv;

  constexpr const auto COMPUTER = "Computer"_sv;

  constexpr const auto DATE = "Date"_sv;

  constexpr const auto COMMENT = "Comment"_sv;

  constexpr const auto FORMAT_SPECIFIC = "FormatSpec"_sv;

  constexpr const auto OFFSET = "Offset"_sv;

  constexpr const auto YM = "YM"_sv;

  constexpr const auto ABC = "ABC"_sv;

  constexpr const auto ACB = "ACB"_sv;

  constexpr const auto BAC = "BAC"_sv;

  constexpr const auto BCA = "BCA"_sv;

  constexpr const auto CAB = "CAB"_sv;

  constexpr const auto CBA = "CBA"_sv;

  constexpr const auto MONO = "Mono"_sv;
}  // namespace AYL
