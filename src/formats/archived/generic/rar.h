/**
 *
 * @file
 *
 * @brief  RAR archives structures
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "byteorder.h"
#include "string_view.h"

namespace Formats::Archived::Rar
{
  struct BlockHeader
  {
    le_uint16_t CRC;
    uint8_t Type;
    le_uint16_t Flags;
    le_uint16_t Size;

    enum
    {
      FLAG_HAS_TAIL = 0x8000,

      MIN_TYPE = 0x72,
      MAX_TYPE = 0x7b
    };

    bool IsExtended() const
    {
      return 0 != (Flags & FLAG_HAS_TAIL);
    }
  };

  struct ExtendedBlockHeader
  {
    BlockHeader Block;
    le_uint32_t AdditionalSize;
  };

  // File header is always ExtendedBlockHeader
  struct FileBlockHeader
  {
    static const uint8_t TYPE = 0x74;

    ExtendedBlockHeader Extended;
    // CRC from Type to Attributes+
    le_uint32_t UnpackedSize;
    uint8_t HostOS;
    le_uint32_t UnpackedCRC;
    le_uint32_t TimeStamp;
    uint8_t DepackerVersion;
    uint8_t Method;
    le_uint16_t NameSize;
    le_uint32_t Attributes;

    enum
    {
      FLAG_SPLIT_BEFORE = 1,
      FLAG_SPLIT_AFTER = 2,
      FLAG_ENCRYPTED = 4,
      FLAG_HAS_COMMENT = 8,
      FLAG_SOLID = 0x10,
      FLAG_DIRECTORY = 0xe0,
      FLAG_BIG_FILE = 0x100,
      FLAG_UNICODE_FILENAME = 0x200,

      MIN_VERSION = 15,
      MAX_VERSION = 36
    };

    bool IsBigFile() const
    {
      return 0 != (Extended.Block.Flags & FLAG_BIG_FILE);
    }

    bool IsSolid() const
    {
      return 0 != (Extended.Block.Flags & FLAG_SOLID);
    }

    bool IsStored() const
    {
      return Method == 0x30;
    }
  };

  struct BigFileBlockHeader
  {
    FileBlockHeader File;
    le_uint32_t PackedSizeHi;
    le_uint32_t UnpackedSizeHi;
  };

  static_assert(sizeof(BlockHeader) * alignof(BlockHeader) == 7, "Wrong layout");
  static_assert(sizeof(ExtendedBlockHeader) * alignof(ExtendedBlockHeader) == 11, "Wrong layout");
  static_assert(sizeof(FileBlockHeader) * alignof(FileBlockHeader) == 32, "Wrong layout");
  static_assert(sizeof(BigFileBlockHeader) * alignof(BigFileBlockHeader) == 40, "Wrong layout");
}  // namespace Formats::Archived::Rar
