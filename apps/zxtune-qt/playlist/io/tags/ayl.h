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
  constexpr const auto SUFFIX = ".ayl"sv;

  constexpr const auto SIGNATURE = "ZX Spectrum Sound Chip Emulator Play List File v1."sv;

  constexpr const auto CHIP_FREQUENCY = "ChipFrequency"sv;

  constexpr const auto PLAYER_FREQUENCY = "PlayerFrequency"sv;

  constexpr const auto CHIP_TYPE = "ChipType"sv;

  constexpr const auto CHANNELS_ALLOCATION = "ChannelsAllocation"sv;

  constexpr const auto NAME = "Name"sv;

  constexpr const auto AUTHOR = "Author"sv;

  constexpr const auto PROGRAM = "Program"sv;

  constexpr const auto TRACKER = "Tracker"sv;

  constexpr const auto COMPUTER = "Computer"sv;

  constexpr const auto DATE = "Date"sv;

  constexpr const auto COMMENT = "Comment"sv;

  constexpr const auto FORMAT_SPECIFIC = "FormatSpec"sv;

  constexpr const auto OFFSET = "Offset"sv;

  constexpr const auto YM = "YM"sv;

  constexpr const auto ABC = "ABC"sv;

  constexpr const auto ACB = "ACB"sv;

  constexpr const auto BAC = "BAC"sv;

  constexpr const auto BCA = "BCA"sv;

  constexpr const auto CAB = "CAB"sv;

  constexpr const auto CBA = "CBA"sv;

  constexpr const auto MONO = "Mono"sv;
}  // namespace AYL
