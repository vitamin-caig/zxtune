/**
 *
 * @file
 *
 * @brief  PSF program section parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/emulation/playstationsoundformat.h"

#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>

#include <byteorder.h>
#include <make_ptr.h>

/*
http://patpend.net/technical/psx/exeheader.txt

I know of 3 types (SCE,PS-X,CPE), but 2 of them are almost the same, and share most of the
fields. The CPE structure will be left for another time as i miss some info that is needed to
start parsing objects.

typedef struct _EXE_HEADER_ {
        u_byte id[8];
        u_long text;			// SCE only
        u_long data;			// SCE only
        u_long pc0;
        u_long gp0;			// SCE only
        u_long t_addr;
        u_long t_size;
        u_long d_addr;			// SCE only
        u_long d_size;			// SCE only
        u_long b_addr;			// SCE only
        u_long b_size;			// SCE only
        u_long s_addr;
        u_long s_size;
        u_long SavedSP;
        u_long SavedFP;
        u_long SavedGP;
        u_long SavedRA;
        u_long SavedS0;
} EXE_HEADER;

Explanation

ExeType = { 'SCE EXE' || 'PS-X EXE' };
text - Offset of the text segment
data - Offset of the data segment
pc0 - Program Counter.
gp0 - Address of the Global Pointer
t_addr - The address where the text segment is loaded
t_size - The size of the text segment
d_addr - The address where the text segment is loaded
d_size - The size of the data segment
b_addr - The address of the BSS segment
b_size - The size of the BSS segment
s_addr - The address of the stack
s_size - The size of the stack.
SavedXX -The Exec system call saves some registers to these fields before jumping to the program

si17911@ci.uminho.pt

*/

namespace Formats::Chiptune
{
  namespace PlaystationSoundFormat
  {
    const Debug::Stream Dbg("Formats::Chiptune::PSF");

    const auto DESCRIPTION = "Playstation Sound Format"sv;

    const std::size_t HEADER_SIZE = 2048;

    class Format
    {
    public:
      explicit Format(const Binary::View& data)
        : Stream(data)
      {}

      void Parse(Builder& target)
      {
        ParseSignature();
        ParseRegisters(target);
        ParseTextSection(target);
        ParseStackSection(target);
        ParseRegion(target);
      }

    private:
      void ParseSignature()
      {
        static const char IDENTIFIER[8] = {'P', 'S', '-', 'X', ' ', 'E', 'X', 'E'};
        Stream.Seek(0);
        Require(0 == std::memcmp(Stream.ReadData(sizeof(IDENTIFIER)).Start(), IDENTIFIER, sizeof(IDENTIFIER)));
      }

      void ParseRegisters(Builder& target)
      {
        Stream.Seek(0x10);
        const uint32_t pc = Stream.Read<le_uint32_t>();
        const uint32_t gp = Stream.Read<le_uint32_t>();
        Dbg("PC=0x{:08x} GP=0x{:08x}", pc, gp);
        target.SetRegisters(pc, gp);
      }

      void ParseTextSection(Builder& target)
      {
        Stream.Seek(0x18);
        const uint32_t startAddress = Stream.Read<le_uint32_t>();
        const std::size_t size = Stream.Read<le_uint32_t>();
        Dbg("Text section {} ({} in header) bytes at 0x{:08x}", Stream.GetRestSize(), size, startAddress);
        if (size)
        {
          Stream.Seek(HEADER_SIZE);
          target.SetTextSection(startAddress, Stream.ReadRestData());
        }
      }

      void ParseStackSection(Builder& target)
      {
        Stream.Seek(0x30);
        const uint32_t stackHead = Stream.Read<le_uint32_t>();
        const uint32_t stackSize = Stream.Read<le_uint32_t>();
        Dbg("Stack {} bytes at 0x{:08x}", stackSize, stackHead);
        target.SetStackRegion(stackHead, stackSize);
      }

      void ParseRegion(Builder& target)
      {
        static const auto MARKER_NORTH_AMERICA = "Sony Computer Entertainment Inc. for North America area"sv;
        static const auto MARKER_JAPAN = "Sony Computer Entertainment Inc. for Japan area"sv;
        static const auto MARKER_EUROPE = "Sony Computer Entertainment Inc. for Europe area"sv;
        Stream.Seek(0x4c);
        const auto marker = Stream.ReadCString(60);
        if (marker == MARKER_NORTH_AMERICA)
        {
          target.SetRegion("North America", 60);
        }
        else if (marker == MARKER_JAPAN)
        {
          target.SetRegion("Japan", 60);
        }
        else if (marker == MARKER_EUROPE)
        {
          target.SetRegion("Europe", 50);
        }
        else
        {
          target.SetRegion(marker, 0);
        }
        Dbg("Marker: {}", marker);
      }

    private:
      Binary::DataInputStream Stream;
    };

    void ParsePSXExe(Binary::View data, Builder& target)
    {
      Format(data).Parse(target);
    }

    const auto FORMAT =
        "'P'S'F"
        "01"
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
      {
        return {};  // TODO
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace PlaystationSoundFormat

  Decoder::Ptr CreatePSFDecoder()
  {
    return MakePtr<PlaystationSoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
