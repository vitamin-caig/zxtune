/**
 *
 * @file
 *
 * @brief  ZIP archives structures
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <byteorder.h>
// std includes
#include <cstddef>

namespace Formats::Packed::Zip
{
  struct GenericHeader
  {
    static const uint16_t SIGNATURE = 0x4b50;

    //+0
    le_uint16_t Signature;
  };

  struct FileAttributes
  {
    le_uint32_t CRC;
    le_uint32_t CompressedSize;
    le_uint32_t UncompressedSize;
  };

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

  struct LocalFileHeader
  {
    static const uint32_t SIGNATURE = 0x04034b50;

    //+0
    le_uint32_t Signature;
    //+4
    le_uint16_t VersionToExtract;
    //+6
    le_uint16_t Flags;
    //+8
    le_uint16_t CompressionMethod;
    //+a
    le_uint16_t ModificationTime;
    //+c
    le_uint16_t ModificationDate;
    //+e
    FileAttributes Attributes;
    //+1a
    le_uint16_t NameSize;
    //+1c
    le_uint16_t ExtraSize;
    //+1e
    char Name[1];

    bool IsValid() const
    {
      return Signature == SIGNATURE;
    }

    std::size_t GetSize() const
    {
      return offsetof(LocalFileHeader, Name) + NameSize + ExtraSize;
    }

    bool IsSupported() const
    {
      return 0 == (Flags & FILE_CRYPTED);
    }
  };

  struct LocalFileFooter
  {
    static const uint32_t SIGNATURE = 0x08074b50;

    //+0
    le_uint32_t Signature;
    //+4
    FileAttributes Attributes;
  };

  struct ExtraDataRecord
  {
    static const uint32_t SIGNATURE = 0x08064b50;

    //+0
    le_uint32_t Signature;
    //+4
    le_uint32_t DataSize;
    //+8
    uint8_t Data[1];

    std::size_t GetSize() const
    {
      return sizeof(*this) - 1 + DataSize;
    }
  };

  struct CentralDirectoryFileHeader
  {
    static const uint32_t SIGNATURE = 0x02014b50;

    //+0
    le_uint32_t Signature;
    //+4
    le_uint16_t VersionMadeBy;
    //+6
    le_uint16_t VersionToExtract;
    //+8
    le_uint16_t Flags;
    //+a
    le_uint16_t CompressionMethod;
    //+c
    le_uint16_t ModificationTime;
    //+e
    le_uint16_t ModificationDate;
    //+10
    FileAttributes Attributes;
    //+1c
    le_uint16_t NameSize;
    //+1e
    le_uint16_t ExtraSize;
    //+20
    le_uint16_t CommentSize;
    //+22
    le_uint16_t StartDiskNumber;
    //+24
    le_uint16_t InternalFileAttributes;
    //+26
    le_uint32_t ExternalFileAttributes;
    //+2a
    le_uint32_t LocalHeaderRelOffset;
    //+2e
    char Name[1];

    std::size_t GetSize() const
    {
      return offsetof(CentralDirectoryFileHeader, Name) + NameSize + ExtraSize + CommentSize;
    }
  };

  struct CentralDirectoryEnd
  {
    static const uint32_t SIGNATURE = 0x06054b50;

    //+0
    le_uint32_t Signature;
    //+4
    le_uint16_t ThisDiskNumber;
    //+6
    le_uint16_t StartDiskNumber;
    //+8
    le_uint16_t ThisDiskCentralDirectoriesCount;
    //+a
    le_uint16_t TotalDirectoriesCount;
    //+c
    le_uint32_t CentralDirectorySize;
    //+10
    le_uint32_t CentralDirectoryOffset;
    //+14
    le_uint16_t CommentSize;
    //+16
    // uint8_t Comment[0];

    std::size_t GetSize() const
    {
      return sizeof(*this) + CommentSize;
    }
  };

  struct DigitalSignature
  {
    static const uint32_t SIGNATURE = 0x05054b50;

    //+0
    le_uint32_t Signature;
    //+4
    le_uint16_t DataSize;
    //+6
    uint8_t Data[1];

    std::size_t GetSize() const
    {
      return sizeof(*this) - 1 + DataSize;
    }
  };

  class CompressedFile
  {
  public:
    virtual ~CompressedFile() = default;

    virtual std::size_t GetPackedSize() const = 0;
    virtual std::size_t GetUnpackedSize() const = 0;

    static std::unique_ptr<const CompressedFile> Create(const LocalFileHeader& hdr, std::size_t availSize);
  };

  static_assert(sizeof(GenericHeader) * alignof(GenericHeader) == 2, "Wrong layout");
  static_assert(sizeof(FileAttributes) * alignof(FileAttributes) == 12, "Wrong layout");
  static_assert(sizeof(LocalFileHeader) * alignof(LocalFileHeader) == 0x1f, "Wrong layout");
  static_assert(sizeof(LocalFileFooter) * alignof(LocalFileFooter) == 16, "Wrong layout");
  static_assert(sizeof(ExtraDataRecord) * alignof(ExtraDataRecord) == 9, "Wrong layout");
  static_assert(sizeof(CentralDirectoryFileHeader) * alignof(CentralDirectoryFileHeader) == 0x2f, "Wrong layout");
  static_assert(sizeof(CentralDirectoryEnd) * alignof(CentralDirectoryEnd) == 0x16, "Wrong layout");
  static_assert(sizeof(DigitalSignature) * alignof(DigitalSignature) == 7, "Wrong layout");
}  // namespace Formats::Packed::Zip
