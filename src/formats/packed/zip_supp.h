/*
Abstract:
  Zip structures declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __FORMATS_PACKED_ZIP_SUPP_H_DEFINED__
#define __FORMATS_PACKED_ZIP_SUPP_H_DEFINED__

//common includes
#include <byteorder.h>

namespace Formats
{
  namespace Packed
  {
    namespace Zip
    {
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
      PACK_PRE struct FileAttributes
      {
        uint32_t CRC;
        uint32_t CompressedSize;
        uint32_t UncompressedSize;
      } PACK_POST;

      enum
      {
        FILE_CRYPTED = 0x1,
        FILE_COMPRESSION_LEVEL_MASK = 0x6,
        FILE_COMPRESSION_LEVEL_NONE = 0x0,
        FILE_COMPRESSION_LEVEL_NORMAL = 0x2,
        FILE_COMPRESSION_LEVEL_FAST = 0x4,
        FILE_COMPRESSION_LEVEL_EXTRAFAST = 0x6,
        FILE_ATTRIBUTES_IN_FOOTER = 0x8,
        FILE_UTF8 = 0x800
      };

      PACK_PRE struct LocalFileHeader
      {
        //+0
        uint32_t Signature; //0x04034b50
        //+4
        uint16_t VersionToExtract;
        //+6
        uint16_t Flags;
        //+8
        uint16_t CompressionMethod;
        //+a
        uint16_t ModificationTime;
        //+c
        uint16_t ModificationDate;
        //+e
        FileAttributes Attributes;
        //+1a
        uint16_t NameSize;
        //+1c
        uint16_t ExtraSize;
        //+1e
        uint8_t Name[1];


        bool IsValid() const;

        std::size_t GetSize() const;

        std::size_t GetTotalFileSize() const;

        bool IsSupported() const;
      } PACK_POST;

      PACK_PRE struct LocalFileFooter
      {
        //+0
        uint32_t Signature; //0x08074b50
        //+4
        FileAttributes Attributes;
      } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
    }
  }
}

#endif //__FORMATS_PACKED_ZIP_SUPP_H_DEFINED__
