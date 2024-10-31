/**
 *
 * @file
 *
 * @brief  ZX-State snapshots structures
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "byteorder.h"
#include "pointers.h"
#include "types.h"

#include <array>

namespace Formats::Archived::ZXState
{
  using Identifier = std::array<uint8_t, 4>;

  const std::size_t UNKNOWN = ~std::size_t(0);

  struct Header
  {
    constexpr static const Identifier SIGNATURE = {{'Z', 'X', 'S', 'T'}};

    Identifier Id;
    uint8_t Major;
    uint8_t Minor;
    uint8_t Machine;
    uint8_t Flags;

    std::size_t GetTotalSize() const
    {
      return sizeof(*this);
    }
  };

  struct Chunk
  {
    Identifier Id;
    le_uint32_t Size;

    std::size_t GetTotalSize() const
    {
      return sizeof(*this) + Size;
    }

    template<class T>
    const T* IsA() const
    {
      return Id == T::SIGNATURE ? static_cast<const T*>(this) : nullptr;
    }
  };

  struct ChunkMempage : Chunk
  {
    static const uint16_t COMPRESSED = 1;

    le_uint16_t Flags;
    uint8_t Number;
    uint8_t Data[1];
  };

  struct ChunkATASP : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'Z', 'X', 'A', 'T'}};

    le_uint16_t Flags;
    uint8_t Ports[3];
    uint8_t Control;
    uint8_t Pages;
    uint8_t ActivePage;
  };

  struct ChunkATASPRAM : ChunkMempage
  {
    constexpr static const Identifier SIGNATURE = {{'A', 'T', 'R', 'P'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x4000;
    }
  };

  struct ChunkAYBLOCK : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'A', 'Y', 0, 0}};

    uint8_t Flags;
    uint8_t CurRegister;
    uint8_t Registers[16];
  };

  struct ChunkBETA128 : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'B', '1', '2', '8'}};
    static const uint32_t EMBEDDED = 2;
    static const uint32_t COMPRESSED = 32;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) ? 0x4000 : 0;
    }

    le_uint32_t Flags;
    uint8_t Drives;
    uint8_t SysReg;
    uint8_t TrackReg;
    uint8_t SectorReg;
    uint8_t DataReg;
    uint8_t StatusReg;
    uint8_t Data[1];
  };

  struct ChunkBETADISK : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'B', 'D', 'S', 'K'}};
    static const uint32_t EMBEDDED = 1;
    static const uint32_t COMPRESSED = 2;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) ? UNKNOWN : 0;
    }

    le_uint32_t Flags;
    uint8_t Number;
    uint8_t Cylinder;
    uint8_t Type;
    uint8_t Data[1];
  };

  struct ChunkCF : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'Z', 'X', 'C', 'F'}};

    le_uint16_t Flags;
    uint8_t ControlReg;
    uint8_t Pages;
  };

  struct ChunkCFRAM : ChunkMempage
  {
    constexpr static const Identifier SIGNATURE = {{'C', 'F', 'R', 'P'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x4000;
    }
  };

  struct ChunkCOVOX : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'C', 'O', 'V', 'X'}};

    uint8_t Volume;
    uint8_t Reserved[3];
  };

  struct ChunkCREATOR : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'C', 'R', 'T', 'R'}};

    char Creator[32];
    le_uint16_t Major;
    le_uint16_t Minor;
    uint8_t Data[1];
  };

  struct ChunkDIVIDE : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'D', 'I', 'D', 'E'}};
    static const uint16_t COMPRESSED = 4;

    static std::size_t GetUncompressedSize()
    {
      return 0x2000;
    }

    le_uint16_t Flags;
    uint8_t ControlRegister;
    uint8_t Pages;
    uint8_t Data[1];
  };

  struct ChunkDIVIDERAMPAGE : ChunkMempage
  {
    constexpr static const Identifier SIGNATURE = {{'D', 'I', 'R', 'P'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x2000;
    }
  };

  struct ChunkDOCK : ChunkMempage
  {
    constexpr static const Identifier SIGNATURE = {{'D', 'O', 'C', 'K'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x2000;
    }
  };

  struct ChunkDSKFILE : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'D', 'S', 'K', 0}};
    static const uint16_t COMPRESSED = 1;
    static const uint16_t EMBEDDED = 2;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) ? std::size_t(UncompressedSize) : 0;
    }

    le_uint16_t Flags;
    uint8_t Number;
    le_uint32_t UncompressedSize;
    uint8_t Data[1];
  };

  struct ChunkGS : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'G', 'S', 0, 0}};
    static const uint8_t EMBEDDED = 64;
    static const uint8_t COMPRESSED = 128;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) ? 0x8000 : 0;
    }

    uint8_t Model;
    uint8_t UpperPage;
    uint8_t Volume[4];
    uint8_t Level[4];
    uint8_t Flags;
    le_uint16_t Registers[13];  // af/bc/de/hl/af'/bc'/de'/hl'/ix/iy/sp/pc/ir
    uint8_t Iff[2];
    uint8_t ImMode;
    le_uint32_t CyclesStart;
    uint8_t HoldIntCycles;
    uint8_t BitReg;
    uint8_t Data[1];
  };

  struct ChunkGSRAMPAGE : ChunkMempage
  {
    constexpr static const Identifier SIGNATURE = {{'G', 'S', 'R', 'P'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x8000;
    }
  };

  struct ChunkIF1 : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'I', 'F', '1', 0}};
    static const uint16_t COMPRESSED = 1;

    std::size_t GetUncompressedSize() const
    {
      return UncompressedSize;
    }

    le_uint16_t Flags;
    uint8_t DrivesCount;
    uint8_t Reserved[35];
    le_uint16_t UncompressedSize;
    uint8_t Data[1];
  };

  struct ChunkIF2ROM : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'I', 'F', '2', 'R'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x4000;
    }

    le_uint32_t CompressedSize;
    uint8_t Data[1];
  };

  struct ChunkJOYSTICK : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'J', 'O', 'Y', 0}};

    le_uint32_t Flags;
    uint8_t TypePlayer1;
    uint8_t TypePlayer2;
  };

  struct ChunkKEYBOARD : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'K', 'E', 'Y', 'B'}};

    le_uint32_t Flags;
    uint8_t Joystick;
  };

  struct ChunkMCART : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'M', 'D', 'R', 'V'}};
    static const uint16_t EMBEDDED = 2;
    static const uint16_t COMPRESSED = 1;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) ? std::size_t(UncompressedSize) : 0;
    }

    le_uint16_t Flags;
    uint8_t Number;
    uint8_t DriveRunning;
    le_uint16_t WritePos;
    le_uint16_t Preamble;
    le_uint32_t UncompressedSize;
    uint8_t Data[1];
  };

  struct ChunkMOUSE : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'A', 'M', 'X', 'M'}};

    uint8_t Type;
    uint8_t CtrlA[3];
    uint8_t CtrlB[3];
  };

  struct ChunkMULTIFACE : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'M', 'F', 'C', 'E'}};
    static const uint8_t COMPRESSED = 2;
    static const uint8_t RAM16K = 32;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & RAM16K) ? 0x4000 : 0x2000;
    }

    uint8_t Model;
    uint8_t Flags;
    uint8_t Data[1];
  };

  struct ChunkOPUS : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'O', 'P', 'U', 'S'}};
    static const uint32_t COMPRESSED = 2;
    static const uint32_t CUSTOMROM = 8;
    static const std::size_t RAMSIZE = 0x800;
    static const std::size_t ROMSIZE = 0x2000;

    static std::size_t GetUncompressedSize()
    {
      return RAMSIZE;
    }

    le_uint32_t Flags;
    le_uint32_t RamDataSize;
    le_uint32_t RomDataSize;
    uint8_t Registers[11];
    uint8_t Data[1];
  };

  struct ChunkOPUSDISK : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'O', 'D', 'S', 'K'}};
    static const uint32_t EMBEDDED = 1;
    static const uint32_t COMPRESSED = 2;
    static const uint8_t TYPE_OPD = 0;
    static const uint8_t TYPE_OPU = 1;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) && (Type == TYPE_OPD || Type == TYPE_OPU) ? UNKNOWN : 0;
    }

    le_uint32_t Flags;
    uint8_t Number;
    uint8_t Cylinder;
    uint8_t Type;
    uint8_t Data[1];
  };

  struct ChunkPLTT : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'P', 'L', 'T', 'T'}};

    // structure is unkown
  };

  struct ChunkPLUS3 : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'+', '3', 0, 0}};

    uint8_t Drives;
    uint8_t MotorOn;
  };

  struct ChunkPLUSD : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'P', 'L', 'S', 'D'}};
    static const uint32_t COMPRESSED = 2;
    static const uint8_t CUSTOMROM = 2;
    static const std::size_t RAMSIZE = 0x2000;
    static const std::size_t ROMSIZE = 0x2000;

    static std::size_t GetUncompressedSize()
    {
      return RAMSIZE;
    }

    le_uint32_t Flags;
    le_uint32_t RamDataSize;
    le_uint32_t RomDataSize;
    uint8_t RomType;
    uint8_t ControlReg;
    uint8_t Drives;
    uint8_t TrackReg;
    uint8_t SectorReg;
    uint8_t DataReg;
    uint8_t StatusReg;
    uint8_t Data[1];
  };

  struct ChunkPLUSDDISK : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'P', 'D', 'S', 'K'}};
    static const uint32_t EMBEDDED = 1;
    static const uint32_t COMPRESSED = 2;
    static const uint8_t TYPE_MGT = 0;
    static const uint8_t TYPE_IMG = 1;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) && (Type == TYPE_MGT || Type == TYPE_IMG) ? UNKNOWN : 0;
    }

    le_uint32_t Flags;
    uint8_t Number;
    uint8_t Cylinder;
    uint8_t Type;
    uint8_t Data[1];
  };

  struct ChunkRAMPAGE : ChunkMempage
  {
    constexpr static const Identifier SIGNATURE = {{'R', 'A', 'M', 'P'}};

    static std::size_t GetUncompressedSize()
    {
      return 0x4000;
    }
  };

  struct ChunkROM : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'R', 'O', 'M', 0}};
    static const uint16_t COMPRESSED = 1;

    std::size_t GetUncompressedSize() const
    {
      return UncompressedSize;
    }

    le_uint16_t Flags;
    le_uint32_t UncompressedSize;
    uint8_t Data[1];
  };

  struct ChunkSCLD : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'S', 'C', 'L', 'D'}};

    uint8_t PortF4;
    uint8_t PortFF;
  };

  struct ChunkSIDE : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'S', 'I', 'D', 'E'}};
  };

  struct ChunkSPECDRUM : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'D', 'R', 'U', 'M'}};

    uint8_t Volume;
  };

  struct ChunkSPECREGS : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'S', 'P', 'C', 'R'}};

    uint8_t Border;
    uint8_t Port7ffd;
    uint8_t Port1ffd_eff7;
    uint8_t PortFe;
    uint8_t Reserved[4];
  };

  struct ChunkSPECTRANET : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'S', 'N', 'E', 'T'}};
    static const uint16_t COMPRESSED = 128;
    static const uint16_t COMPRESSED_RAM = 256;
    static const std::size_t DUMPSIZE = 0x20000;

    le_uint16_t Flags;
    uint8_t PageA;
    uint8_t PageB;
    le_uint16_t Trap;
    uint8_t W5100[0x30];
    le_uint32_t FlashSize;
    uint8_t Data[1];  // flash+ram prepend with size
  };

  struct ChunkTAPE : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'T', 'A', 'P', 'E'}};
    static const uint16_t EMBEDDED = 1;
    static const uint16_t COMPRESSED = 2;

    std::size_t GetUncompressedSize() const
    {
      return 0 != (Flags & EMBEDDED) ? std::size_t(UncompressedSize) : 0;
    }

    le_uint16_t Number;
    le_uint16_t Flags;
    le_uint32_t UncompressedSize;
    le_uint32_t CompressedSize;
    char Extension[16];
    uint8_t Data[1];
  };

  struct ChunkUSPEECH : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'U', 'S', 'P', 'E'}};

    uint8_t PagedIn;
  };

  struct ChunkZXPRINTER : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'Z', 'X', 'P', 'R'}};

    le_uint16_t Flags;
  };

  struct ChunkZ80REGS : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{'Z', '8', '0', 'R'}};

    le_uint16_t Registers[13];  // af/bc/de/hl/af'/bc'/de'/hl'/ix/iy/sp/pc/ir
    uint8_t Iff[2];
    uint8_t ImMode;
    le_uint32_t CyclesStart;
    uint8_t HoldIntCycles;
    uint8_t Flags;
    le_uint16_t Memptr;
  };

  /*
  struct  : Chunk
  {
    constexpr static const Identifier SIGNATURE = {{}};

  };
  */

  static_assert(sizeof(Header) * alignof(Header) == 8, "Invalid layout");
  static_assert(sizeof(Chunk) * alignof(Chunk) == 8, "Invalid layout");
  static_assert(sizeof(ChunkMempage) * alignof(ChunkMempage) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkATASP) * alignof(ChunkATASP) == 8 + 8, "Invalid layout");
  static_assert(sizeof(ChunkATASPRAM) * alignof(ChunkATASPRAM) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkAYBLOCK) * alignof(ChunkAYBLOCK) == 8 + 18, "Invalid layout");
  static_assert(sizeof(ChunkBETA128) * alignof(ChunkBETA128) == 8 + 11, "Invalid layout");
  static_assert(sizeof(ChunkBETADISK) * alignof(ChunkBETADISK) == 8 + 8, "Invalid layout");
  static_assert(sizeof(ChunkCF) * alignof(ChunkCF) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkCFRAM) * alignof(ChunkCFRAM) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkCOVOX) * alignof(ChunkCOVOX) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkCREATOR) * alignof(ChunkCREATOR) == 8 + 37, "Invalid layout");
  static_assert(sizeof(ChunkDIVIDE) * alignof(ChunkDIVIDE) == 8 + 5, "Invalid layout");
  static_assert(sizeof(ChunkDIVIDERAMPAGE) * alignof(ChunkDIVIDERAMPAGE) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkDOCK) * alignof(ChunkDOCK) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkDSKFILE) * alignof(ChunkDSKFILE) == 8 + 8, "Invalid layout");
  static_assert(sizeof(ChunkGS) * alignof(ChunkGS) == 8 + 47, "Invalid layout");
  static_assert(sizeof(ChunkGSRAMPAGE) * alignof(ChunkGSRAMPAGE) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkIF1) * alignof(ChunkIF1) == 8 + 41, "Invalid layout");
  static_assert(sizeof(ChunkIF2ROM) * alignof(ChunkIF2ROM) == 8 + 5, "Invalid layout");
  static_assert(sizeof(ChunkJOYSTICK) * alignof(ChunkJOYSTICK) == 8 + 6, "Invalid layout");
  static_assert(sizeof(ChunkKEYBOARD) * alignof(ChunkKEYBOARD) == 8 + 5, "Invalid layout");
  static_assert(sizeof(ChunkMCART) * alignof(ChunkMCART) == 8 + 13, "Invalid layout");
  static_assert(sizeof(ChunkMOUSE) * alignof(ChunkMOUSE) == 8 + 7, "Invalid layout");
  static_assert(sizeof(ChunkMULTIFACE) * alignof(ChunkMULTIFACE) == 8 + 3, "Invalid layout");
  static_assert(sizeof(ChunkOPUS) * alignof(ChunkOPUS) == 8 + 24, "Invalid layout");
  static_assert(sizeof(ChunkOPUSDISK) * alignof(ChunkOPUSDISK) == 8 + 8, "Invalid layout");
  static_assert(sizeof(ChunkPLUS3) * alignof(ChunkPLUS3) == 8 + 2, "Invalid layout");
  static_assert(sizeof(ChunkPLUSD) * alignof(ChunkPLUSD) == 8 + 20, "Invalid layout");
  static_assert(sizeof(ChunkPLUSDDISK) * alignof(ChunkPLUSDDISK) == 8 + 8, "Invalid layout");
  static_assert(sizeof(ChunkRAMPAGE) * alignof(ChunkRAMPAGE) == 8 + 4, "Invalid layout");
  static_assert(sizeof(ChunkROM) * alignof(ChunkROM) == 8 + 7, "Invalid layout");
  static_assert(sizeof(ChunkSCLD) * alignof(ChunkSCLD) == 8 + 2, "Invalid layout");
  static_assert(sizeof(ChunkSIDE) * alignof(ChunkSIDE) == 8 + 0, "Invalid layout");
  static_assert(sizeof(ChunkSPECDRUM) * alignof(ChunkSPECDRUM) == 8 + 1, "Invalid layout");
  static_assert(sizeof(ChunkSPECREGS) * alignof(ChunkSPECREGS) == 8 + 8, "Invalid layout");
  static_assert(sizeof(ChunkSPECTRANET) * alignof(ChunkSPECTRANET) == 8 + 59, "Invalid layout");
  static_assert(sizeof(ChunkTAPE) * alignof(ChunkTAPE) == 8 + 29, "Invalid layout");
  static_assert(sizeof(ChunkUSPEECH) * alignof(ChunkUSPEECH) == 8 + 1, "Invalid layout");
  static_assert(sizeof(ChunkZXPRINTER) * alignof(ChunkZXPRINTER) == 8 + 2, "Invalid layout");
  static_assert(sizeof(ChunkZ80REGS) * alignof(ChunkZ80REGS) == 8 + 37, "Invalid layout");

  // unified field access traits
  template<class ChunkType>
  struct ChunkTraits
  {
    static const uint8_t* GetData(const ChunkType& ch)
    {
      return ch.Data;
    }

    static std::size_t GetDataSize(const ChunkType& ch)
    {
      return safe_ptr_cast<const uint8_t*>(&ch) + ch.GetTotalSize() - ch.Data;
    }

    static bool IsDataCompressed(const ChunkType& ch)
    {
      return 0 != (ch.Flags & ch.COMPRESSED);
    }

    static std::size_t GetTargetSize(const ChunkType& ch)
    {
      return ch.GetUncompressedSize();
    }

    static uint_t GetIndex(const ChunkType& ch)
    {
      return ch.Number;
    }
  };
}  // namespace Formats::Archived::ZXState
