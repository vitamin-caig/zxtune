/*
Abstract:
  ZX-State snapshots structures declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef FORMATS_ARCHIVED_ZXSTATE_SUPP_H
#define FORMATS_ARCHIVED_ZXSTATE_SUPP_H

//common includes
#include <byteorder.h>
#include <pointers.h>
#include <types.h>

namespace Formats
{
  namespace Archived
  {
    namespace ZXState
    {
      template<uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4>
      struct Sequence
      {
        static const uint32_t Value = (uint32_t(c4) << 24) | (uint32_t(c3) << 16) | (uint32_t(c2) << 8) | uint32_t(c1);
      };

      const std::size_t UNKNOWN = ~std::size_t(0);

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
      PACK_PRE struct Header
      {
        static const uint32_t SIGNATURE = Sequence<'Z', 'X', 'S', 'T'>::Value;

        uint32_t Id;
        uint8_t Major;
        uint8_t Minor;
        uint8_t Machine;
        uint8_t Flags;

        std::size_t GetTotalSize() const
        {
          return sizeof(*this);
        }
      } PACK_POST;

      PACK_PRE struct Chunk
      {
        uint32_t Id;
        uint32_t Size;

        std::size_t GetTotalSize() const
        {
          return sizeof(*this) + fromLE(Size);
        }

        template<class T>
        const T* IsA() const
        {
          return Id == T::SIGNATURE
            ? static_cast<const T*>(this)
            : 0;
        }
      } PACK_POST;

      PACK_PRE struct ChunkMempage : Chunk
      {
        static const uint16_t COMPRESSED = 1;

        uint16_t Flags;
        uint8_t Number;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkATASP : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'Z', 'X', 'A', 'T'>::Value;

        uint16_t Flags;
        uint8_t Ports[3];
        uint8_t Control;
        uint8_t Pages;
        uint8_t ActivePage;
      } PACK_POST;

      PACK_PRE struct ChunkATASPRAM : ChunkMempage
      {
        static const uint32_t SIGNATURE = Sequence<'A', 'T', 'R', 'P'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x4000;
        }
      } PACK_POST;

      PACK_PRE struct ChunkAYBLOCK : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'A', 'Y', 0, 0>::Value;

        uint8_t Flags;
        uint8_t CurRegister;
        uint8_t Registers[16];
      } PACK_POST;

      PACK_PRE struct ChunkBETA128 : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'B', '1', '2', '8'>::Value;
        static const uint32_t EMBEDDED = 2;
        static const uint32_t COMPRESSED = 32;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED)
            ? 0x4000
            : 0;
        }

        uint32_t Flags;
        uint8_t Drives;
        uint8_t SysReg;
        uint8_t TrackReg;
        uint8_t SectorReg;
        uint8_t DataReg;
        uint8_t StatusReg;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkBETADISK : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'B', 'D', 'S', 'K'>::Value;
        static const uint32_t EMBEDDED = 1;
        static const uint32_t COMPRESSED = 2;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED)
            ? UNKNOWN
            : 0;
        }

        uint32_t Flags;
        uint8_t Number;
        uint8_t Cylinder;
        uint8_t Type;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkCF : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'Z', 'X', 'C', 'F'>::Value;

        uint16_t Flags;
        uint8_t ControlReg;
        uint8_t Pages;
      } PACK_POST;

      PACK_PRE struct ChunkCFRAM : ChunkMempage
      {
        static const uint32_t SIGNATURE = Sequence<'C', 'F', 'R', 'P'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x4000;
        }
      } PACK_POST;

      PACK_PRE struct ChunkCOVOX : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'C', 'O', 'V', 'X'>::Value;

        uint8_t Volume;
        uint8_t Reserved[3];
      } PACK_POST;

      PACK_PRE struct ChunkCREATOR : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'C', 'R', 'T', 'R'>::Value;

        char Creator[32];
        uint16_t Major;
        uint16_t Minor;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkDIVIDE : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'D', 'I', 'D', 'E'>::Value;
        static const uint16_t COMPRESSED = 4;

        static std::size_t GetUncompressedSize()
        {
          return 0x2000;
        }

        uint16_t Flags;
        uint8_t ControlRegister;
        uint8_t Pages;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkDIVIDERAMPAGE : ChunkMempage
      {
        static const uint32_t SIGNATURE = Sequence<'D', 'I', 'R', 'P'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x2000;
        }
      } PACK_POST;

      PACK_PRE struct ChunkDOCK : ChunkMempage
      {
        static const uint32_t SIGNATURE = Sequence<'D', 'O', 'C', 'K'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x2000;
        }
      } PACK_POST;

      PACK_PRE struct ChunkDSKFILE : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'D', 'S', 'K', 0>::Value;
        static const uint16_t COMPRESSED = 1;
        static const uint16_t EMBEDDED = 2;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED)
            ? fromLE(UncompressedSize)
            : 0;
        }

        uint16_t Flags;
        uint8_t Number;
        uint32_t UncompressedSize;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkGS : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'G', 'S', 0, 0>::Value;
        static const uint8_t EMBEDDED = 64;
        static const uint8_t COMPRESSED = 128;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (Flags & EMBEDDED)
            ? 0x8000
            : 0;
        }

        uint8_t Model;
        uint8_t UpperPage;
        uint8_t Volume[4];
        uint8_t Level[4];
        uint8_t Flags;
        uint16_t Registers[13];//af/bc/de/hl/af'/bc'/de'/hl'/ix/iy/sp/pc/ir
        uint8_t Iff[2];
        uint8_t ImMode;
        uint32_t CyclesStart;
        uint8_t HoldIntCycles;
        uint8_t BitReg;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkGSRAMPAGE : ChunkMempage
      {
        static const uint32_t SIGNATURE = Sequence<'G', 'S', 'R', 'P'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x8000;
        }
      } PACK_POST;

      PACK_PRE struct ChunkIF1 : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'I', 'F', '1', 0>::Value;
        static const uint16_t COMPRESSED = 1;

        std::size_t GetUncompressedSize() const
        {
          return fromLE(UncompressedSize);
        }

        uint16_t Flags;
        uint8_t DrivesCount;
        uint8_t Reserved[35];
        uint16_t UncompressedSize;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkIF2ROM : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'I', 'F', '2', 'R'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x4000;
        }

        uint32_t CompressedSize;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkJOYSTICK : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'J', 'O', 'Y', 0>::Value;

        uint32_t Flags;
        uint8_t TypePlayer1;
        uint8_t TypePlayer2;
      } PACK_POST;

      PACK_PRE struct ChunkKEYBOARD : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'K', 'E', 'Y', 'B'>::Value;

        uint32_t Flags;
        uint8_t Joystick;
      } PACK_POST;

      PACK_PRE struct ChunkMCART : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'M', 'D', 'R', 'V'>::Value;
        static const uint16_t EMBEDDED = 2;
        static const uint16_t COMPRESSED = 1;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED)
            ? fromLE(UncompressedSize)
            : 0;
        }

        uint16_t Flags;
        uint8_t Number;
        uint8_t DriveRunning;
        uint16_t WritePos;
        uint16_t Preamble;
        uint32_t UncompressedSize;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkMOUSE : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'A', 'M', 'X', 'M'>::Value;

        uint8_t Type;
        uint8_t CtrlA[3];
        uint8_t CtrlB[3];
      } PACK_POST;

      PACK_PRE struct ChunkMULTIFACE : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'M', 'F', 'C', 'E'>::Value;
        static const uint8_t COMPRESSED = 2;
        static const uint8_t RAM16K = 32;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (Flags & RAM16K)
            ? 0x4000
            : 0x2000;
        }

        uint8_t Model;
        uint8_t Flags;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkOPUS : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'O', 'P', 'U', 'S'>::Value;
        static const uint32_t COMPRESSED = 2;
        static const uint32_t CUSTOMROM = 8;
        static const std::size_t RAMSIZE = 0x800;
        static const std::size_t ROMSIZE = 0x2000;

        static std::size_t GetUncompressedSize()
        {
          return RAMSIZE;
        }

        uint32_t Flags;
        uint32_t RamDataSize;
        uint32_t RomDataSize;
        uint8_t Registers[11];
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkOPUSDISK : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'O', 'D', 'S', 'K'>::Value;
        static const uint32_t EMBEDDED = 1;
        static const uint32_t COMPRESSED = 2;
        static const uint8_t TYPE_OPD = 0;
        static const uint8_t TYPE_OPU = 1;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED) && (Type == TYPE_OPD || Type == TYPE_OPU)
            ? UNKNOWN
            : 0;
        }

        uint32_t Flags;
        uint8_t Number;
        uint8_t Cylinder;
        uint8_t Type;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkPLTT : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'P', 'L', 'T', 'T'>::Value;

        //structure is unkown
      } PACK_POST;

      PACK_PRE struct ChunkPLUS3 : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'+', '3', 0, 0>::Value;

        uint8_t Drives;
        uint8_t MotorOn;
      } PACK_POST;

      PACK_PRE struct ChunkPLUSD : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'P', 'L', 'S', 'D'>::Value;
        static const uint32_t COMPRESSED = 2;
        static const uint8_t CUSTOMROM = 2;
        static const std::size_t RAMSIZE = 0x2000;
        static const std::size_t ROMSIZE = 0x2000;

        static std::size_t GetUncompressedSize()
        {
          return RAMSIZE;
        }

        uint32_t Flags;
        uint32_t RamDataSize;
        uint32_t RomDataSize;
        uint8_t RomType;
        uint8_t ControlReg;
        uint8_t Drives;
        uint8_t TrackReg;
        uint8_t SectorReg;
        uint8_t DataReg;
        uint8_t StatusReg;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkPLUSDDISK : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'P', 'D', 'S', 'K'>::Value;
        static const uint32_t EMBEDDED = 1;
        static const uint32_t COMPRESSED = 2;
        static const uint8_t TYPE_MGT = 0;
        static const uint8_t TYPE_IMG = 1;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED) && (Type == TYPE_MGT || Type == TYPE_IMG)
            ? UNKNOWN
            : 0;
        }

        uint32_t Flags;
        uint8_t Number;
        uint8_t Cylinder;
        uint8_t Type;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkRAMPAGE : ChunkMempage
      {
        static const uint32_t SIGNATURE = Sequence<'R', 'A', 'M', 'P'>::Value;

        static std::size_t GetUncompressedSize()
        {
          return 0x4000;
        }
      } PACK_POST;

      PACK_PRE struct ChunkROM : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'R', 'O', 'M', 0>::Value;
        static const uint16_t COMPRESSED = 1;

        std::size_t GetUncompressedSize() const
        {
          return fromLE(UncompressedSize);
        }

        uint16_t Flags;
        uint32_t UncompressedSize;
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkSCLD : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'S', 'C', 'L', 'D'>::Value;

        uint8_t PortF4;
        uint8_t PortFF;
      } PACK_POST;

      PACK_PRE struct ChunkSIDE : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'S', 'I', 'D', 'E'>::Value;
      } PACK_POST;

      PACK_PRE struct ChunkSPECDRUM : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'D', 'R', 'U', 'M'>::Value;

        uint8_t Volume;
      } PACK_POST;

      PACK_PRE struct ChunkSPECREGS : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'S', 'P', 'C', 'R'>::Value;

        uint8_t Border;
        uint8_t Port7ffd;
        uint8_t Port1ffd_eff7;
        uint8_t PortFe;
        uint8_t Reserved[4];
      } PACK_POST;

      PACK_PRE struct ChunkSPECTRANET : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'S', 'N', 'E', 'T'>::Value;
        static const uint16_t COMPRESSED = 128;
        static const uint16_t COMPRESSED_RAM = 256;
        static const std::size_t DUMPSIZE = 0x20000;

        uint16_t Flags;
        uint8_t PageA;
        uint8_t PageB;
        uint16_t Trap;
        uint8_t W5100[0x30];
        uint32_t FlashSize;
        uint8_t Data[1];//flash+ram prepend with size
      } PACK_POST;

      PACK_PRE struct ChunkTAPE : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'T', 'A', 'P', 'E'>::Value;
        static const uint16_t EMBEDDED = 1;
        static const uint16_t COMPRESSED = 2;

        std::size_t GetUncompressedSize() const
        {
          return 0 != (fromLE(Flags) & EMBEDDED)
            ? fromLE(UncompressedSize)
            : 0;
        }

        uint16_t Number;
        uint16_t Flags;
        uint32_t UncompressedSize;
        uint32_t CompressedSize;
        char Extension[16];
        uint8_t Data[1];
      } PACK_POST;

      PACK_PRE struct ChunkUSPEECH : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'U', 'S', 'P', 'E'>::Value;

        uint8_t PagedIn;
      } PACK_POST;

      PACK_PRE struct ChunkZXPRINTER : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'Z', 'X', 'P', 'R'>::Value;

        uint16_t Flags;
      } PACK_POST;

      PACK_PRE struct ChunkZ80REGS : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<'Z', '8', '0', 'R'>::Value;

        uint16_t Registers[13];//af/bc/de/hl/af'/bc'/de'/hl'/ix/iy/sp/pc/ir
        uint8_t Iff[2];
        uint8_t ImMode;
        uint32_t CyclesStart;
        uint8_t HoldIntCycles;
        uint8_t Flags;
        uint16_t Memptr;
      } PACK_POST;

      /*
      PACK_PRE struct  : Chunk
      {
        static const uint32_t SIGNATURE = Sequence<>::Value;

      } PACK_POST;
      */
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

      BOOST_STATIC_ASSERT(sizeof(Header) == 8);
      BOOST_STATIC_ASSERT(sizeof(Chunk) == 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkMempage) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkATASP) == 8 + 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkATASPRAM) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkAYBLOCK) == 8 + 18);
      BOOST_STATIC_ASSERT(sizeof(ChunkBETA128) == 8 + 11);
      BOOST_STATIC_ASSERT(sizeof(ChunkBETADISK) == 8 + 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkCF) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkCFRAM) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkCOVOX) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkCREATOR) == 8 + 37);
      BOOST_STATIC_ASSERT(sizeof(ChunkDIVIDE) == 8 + 5);
      BOOST_STATIC_ASSERT(sizeof(ChunkDIVIDERAMPAGE) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkDOCK) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkDSKFILE) == 8 + 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkGS) == 8 + 47);
      BOOST_STATIC_ASSERT(sizeof(ChunkGSRAMPAGE) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkIF1) == 8 + 41);
      BOOST_STATIC_ASSERT(sizeof(ChunkIF2ROM) == 8 + 5);
      BOOST_STATIC_ASSERT(sizeof(ChunkJOYSTICK) == 8 + 6);
      BOOST_STATIC_ASSERT(sizeof(ChunkKEYBOARD) == 8 + 5);
      BOOST_STATIC_ASSERT(sizeof(ChunkMCART) == 8 + 13);
      BOOST_STATIC_ASSERT(sizeof(ChunkMOUSE) == 8 + 7);
      BOOST_STATIC_ASSERT(sizeof(ChunkMULTIFACE) == 8 + 3);
      BOOST_STATIC_ASSERT(sizeof(ChunkOPUS) == 8 + 24);
      BOOST_STATIC_ASSERT(sizeof(ChunkOPUSDISK) == 8 + 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkPLUS3) == 8 + 2);
      BOOST_STATIC_ASSERT(sizeof(ChunkPLUSD) == 8 + 20);
      BOOST_STATIC_ASSERT(sizeof(ChunkPLUSDDISK) == 8 + 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkRAMPAGE) == 8 + 4);
      BOOST_STATIC_ASSERT(sizeof(ChunkROM) == 8 + 7);
      BOOST_STATIC_ASSERT(sizeof(ChunkSCLD) == 8 + 2);
      BOOST_STATIC_ASSERT(sizeof(ChunkSIDE) == 8 + 0);
      BOOST_STATIC_ASSERT(sizeof(ChunkSPECDRUM) == 8 + 1);
      BOOST_STATIC_ASSERT(sizeof(ChunkSPECREGS) == 8 + 8);
      BOOST_STATIC_ASSERT(sizeof(ChunkSPECTRANET) == 8 + 59);
      BOOST_STATIC_ASSERT(sizeof(ChunkTAPE) == 8 + 29);
      BOOST_STATIC_ASSERT(sizeof(ChunkUSPEECH) == 8 + 1);
      BOOST_STATIC_ASSERT(sizeof(ChunkZXPRINTER) == 8 + 2);
      BOOST_STATIC_ASSERT(sizeof(ChunkZ80REGS) == 8 + 37);

      //unified field access traits
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
          return 0 != (fromLE(ch.Flags) & ch.COMPRESSED);
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
    }
  }
}

#endif //FORMATS_ARCHIVED_ZXSTATE_SUPP_H
