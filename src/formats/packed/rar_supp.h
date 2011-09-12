/*
Abstract:
  Rar structures declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __FORMATS_PACKED_RAR_SUPP_H_DEFINED__
#define __FORMATS_PACKED_RAR_SUPP_H_DEFINED__

//common includes
#include <byteorder.h>

namespace Formats
{
  namespace Packed
  {
    namespace Rar
    {
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
      PACK_PRE struct BlockHeader
      {
        uint16_t CRC;
        uint8_t Type;
        uint16_t Flags;
        uint16_t Size;

        enum
        {
          FLAG_HAS_TAIL = 0x8000,

          MIN_TYPE = 0x72,
          MAX_TYPE = 0x7b
        };

        bool IsExtended() const
        {
          return 0 != (fromLE(Flags) & FLAG_HAS_TAIL);
        }
      } PACK_POST;

      PACK_PRE struct ExtendedBlockHeader : BlockHeader
      {
        uint32_t AdditionalSize;
      } PACK_POST;

      // ArchiveBlockHeader is always BlockHeader
      PACK_PRE struct ArchiveBlockHeader : BlockHeader
      {
        static const uint8_t TYPE = 0x73;

        //CRC from Type till Reserved2
        uint16_t Reserved1;
        uint32_t Reserved2;

        enum
        {
          FLAG_VOLUME = 1,
          FLAG_HAS_COMMENT = 2,
          FLAG_BLOCKED = 4,
          FLAG_SOLID = 8,
          FLAG_SIGNATURE = 0x20,
        };
      } PACK_POST;

      // File header is always ExtendedBlockHeader
      PACK_PRE struct FileBlockHeader : ExtendedBlockHeader
      {
        static const uint8_t TYPE = 0x74;

        //CRC from Type to Attributes+
        uint32_t UnpackedSize;
        uint8_t HostOS;
        uint32_t UnpackedCRC;
        uint32_t TimeStamp;
        uint8_t DepackerVersion;
        uint8_t Method;
        uint16_t NameSize;
        uint32_t Attributes;

        enum
        {
          FLAG_SPLIT_BEFORE = 1,
          FLAG_SPLIT_AFTER = 2,
          FLAG_ENCRYPTED = 4,
          FLAG_HAS_COMMENT = 8,
          FLAG_SOLID = 0x10,
          FLAG_DIRECTORY = 0xe0,
          FLAG_BIG_FILE = 0x100,

          MIN_VERSION = 13,
          MAX_VERSION = 20
        };

        bool IsBigFile() const
        {
          return 0 != (fromLE(Flags) & FLAG_BIG_FILE);
        }

        bool IsValid() const;

        bool IsSupported() const;

        String GetName() const;
      } PACK_POST;

      PACK_PRE struct BigFileBlockHeader : FileBlockHeader
      {
        uint32_t PackedSizeHi;
        uint32_t UnpackedSizeHi;
      } PACK_POST; 
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
    }
  }
}

#endif //__FORMATS_PACKED_RAR_SUPP_H_DEFINED__
