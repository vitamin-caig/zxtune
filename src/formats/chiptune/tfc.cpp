/*
Abstract:
  TFC modules format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "tfc.h"
//common includes
#include <byteorder.h>
//library includes
#include <binary/input_stream.h>
#include <binary/typed_container.h>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace TFC
  {
    typedef boost::array<uint8_t, 6> SignatureType;

    const SignatureType SIGNATURE = { {'T', 'F', 'M', 'c', 'o', 'm'} };

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      SignatureType Sign;
      char Version[3];
      uint8_t IntFreq;
      boost::array<uint16_t, 6> Offsets;
      uint8_t Reserved[12];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif
    const std::size_t MIN_SIZE = sizeof(RawHeader) + 3 + 6;//header + 3 empty strings + 6 finish markers
    const std::size_t MAX_STRING_SIZE = 64;
    const std::size_t MAX_COMMENT_SIZE = 384;

    class StubBuilder : public Builder
    {
    public:
      virtual void SetVersion(const String& /*version*/) {}
      virtual void SetIntFreq(uint_t /*freq*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetAuthor(const String& /*author*/) {}
      virtual void SetComment(const String& /*comment*/) {}

      virtual void StartChannel(uint_t /*idx*/) {}
      virtual void StartFrame() {}
      virtual void SetSkip(uint_t /*count*/) {}
      virtual void SetLoop() {}
      virtual void SetSlide(uint_t /*slide*/) {}
      virtual void SetKeyOff() {}
      virtual void SetFreq(uint_t /*freq*/) {}
      virtual void SetRegister(uint_t /*reg*/, uint_t /*val*/) {}
      virtual void SetKeyOn() {}
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size = rawData.Size();
      if (size < MIN_SIZE)
      {
        return false;
      }
      const RawHeader& hdr = *safe_ptr_cast<const RawHeader*>(rawData.Start());
      return hdr.Sign == SIGNATURE && hdr.Offsets.end() == std::find_if(hdr.Offsets.begin(), hdr.Offsets.end(), boost::bind(&fromLE<uint16_t>, _1) >= size);
    }

    const std::string FORMAT(
      "'T'F'M'c'o'm"
      "???"
      "32|3c"
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::TFC_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    struct Context
    {
      std::size_t RetAddr;
      std::size_t RepeatFrames;

      Context()
        : RetAddr()
        , RepeatFrames()
      {
      }
    };

    class Container
    {
    public:
      explicit Container(const Binary::Container& data)
        : Delegate(data)
        , Min(Delegate.GetSize())
        , Max(0)
      {
      }

      std::size_t ParseFrameControl(std::size_t cursor, Builder& target, Context& context) const
      {
        if (context.RepeatFrames && !--context.RepeatFrames)
        {
          cursor = context.RetAddr;
          context.RetAddr = 0;
        }
        for (;;)
        {
          const uint_t cmd = Get<uint8_t>(cursor++);
          if (cmd == 0x7f)//%01111111
          {
            return 0;
          }
          else if (0x7e == cmd)//%01111110
          {
            target.SetLoop();
            continue;
          }
          else if (0xd0 == cmd)//%11010000
          {
            Require(context.RepeatFrames == 0);
            Require(context.RetAddr == 0);
            context.RepeatFrames = Get<uint8_t>(cursor++);
            const int_t offset = fromBE(Get<int16_t>(cursor));
            context.RetAddr = cursor += 2;
            return cursor + offset;
          }
          else
          {
            return cursor - 1;
          }
        }
      }

      std::size_t ParseFrameCommands(std::size_t cursor, Builder& target) const
      {
        const uint_t cmd = Get<uint8_t>(cursor++);
        if (0xbf == cmd)//%10111111
        {
          const int_t offset = fromBE(Get<int16_t>(cursor));
          cursor += 2;
          ParseFrameData(cursor + offset, target);
        }
        else if (0xff == cmd)//%11111111
        {
          const int_t offset = -256 + Get<uint8_t>(cursor++);
          ParseFrameData(cursor + offset, target);
        }
        else if (cmd >= 0xe0)//%111ttttt
        {
          target.SetSkip(256 - cmd);
        }
        else if (cmd >= 0xc0)//%110ddddd
        {
          target.SetSlide(cmd + 0x30);
        }
        else
        {
          cursor = ParseFrameData(cursor - 1, target);
        }
        return cursor;
      }

      std::size_t GetMin() const
      {
        return Min;
      }

      std::size_t GetMax() const
      {
        return Max;
      }
    private:
      std::size_t ParseFrameData(std::size_t cursor, Builder& target) const
      {
        const uint_t data = Get<uint8_t>(cursor++);
        if (0 != (data & 0xc0))
        {
          target.SetKeyOff();
        }
        if (0 != (data & 0x01))
        {
          const uint_t freq = fromBE(Get<uint16_t>(cursor));
          cursor += 2;
          target.SetFreq(freq);
        }
        if (uint_t regs = (data & 0x3e) >> 1)
        {
          for (uint_t i = 0; i != regs; ++i)
          {
            const uint_t reg = Get<uint8_t>(cursor++);
            const uint_t val = Get<uint8_t>(cursor++);
            target.SetRegister(reg, val);
          }
        }
        if (0 != (data & 0x80))
        {
          target.SetKeyOn();
        }
        return cursor;
      }
    private:
      template<class T>
      const T& Get(std::size_t offset) const
      {
        const T* const ptr = Delegate.GetField<T>(offset);
        Require(ptr != 0);
        Min = std::min(Min, offset);
        Max = std::max(Max, offset + sizeof(T));
        return *ptr;
      }
    private:
      const Binary::TypedContainer Delegate;
      mutable std::size_t Min;
      mutable std::size_t Max;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }
      try
      {
        Binary::InputStream stream(data);
        const RawHeader& header = stream.ReadField<RawHeader>();
        target.SetVersion(FromCharArray(header.Version));
        target.SetIntFreq(header.IntFreq);
        target.SetTitle(FromStdString(stream.ReadCString(MAX_STRING_SIZE)));
        target.SetAuthor(FromStdString(stream.ReadCString(MAX_STRING_SIZE)));
        target.SetComment(FromStdString(stream.ReadCString(MAX_COMMENT_SIZE)));

        const Container container(data);
        for (uint_t chan = 0; chan != 6; ++chan)
        {
          target.StartChannel(chan);
          Context context;
          for (std::size_t cursor = fromLE(header.Offsets[chan]); cursor != 0;)
          {
            if ((cursor = container.ParseFrameControl(cursor, target, context)))
            {
              target.StartFrame();
              cursor = container.ParseFrameCommands(cursor, target);
            }
          }
        }

        const std::size_t usedSize = std::max(container.GetMax(), stream.GetPosition());
        const std::size_t fixedOffset = container.GetMin();
        const Binary::Container::Ptr subData = data.GetSubcontainer(0, usedSize);
        return CreateCalculatingCrcContainer(subData, fixedOffset, usedSize - fixedOffset);
      }
      catch (const std::exception&)
      {
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace TFC

  Decoder::Ptr CreateTFCDecoder()
  {
    return boost::make_shared<TFC::Decoder>();
  }
}//namespace Chiptune
}//namespace Formats
