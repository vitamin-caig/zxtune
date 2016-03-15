/**
* 
* @file
*
* @brief  TurboFM Dump support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfd.h"
#include "formats/chiptune/container.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
//boost includes
#include <boost/array.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace TFD
  {
    enum
    {
      BEGIN_FRAME = 0xff,
      SKIP_FRAMES = 0xfe,
      SELECT_SECOND_CHIP = 0xfd,
      SELECT_FIRST_CHIP = 0xfc,
      FINISH = 0xfb,
      LOOP_MARKER = 0xfa
    };

    const std::size_t MIN_SIZE = 4 + 3 + 1;//header + 3 empty strings + finish marker
    const std::size_t MAX_STRING_SIZE = 64;
    const std::size_t MAX_COMMENT_SIZE = 384;

    typedef boost::array<uint8_t, 4> SignatureType;

    const SignatureType SIGNATURE = { {'T', 'F', 'M', 'D'} };

    class StubBuilder : public Builder
    {
    public:
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetAuthor(const String& /*author*/) {}
      virtual void SetComment(const String& /*comment*/) {}

      virtual void BeginFrames(uint_t /*count*/) {}
      virtual void SelectChip(uint_t /*idx*/) {}
      virtual void SetLoop() {}
      virtual void SetRegister(uint_t /*idx*/, uint_t /*val*/) {}
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      if (rawData.Size() < MIN_SIZE)
      {
        return false;
      }
      const SignatureType& sign = *static_cast<const SignatureType*>(rawData.Start());
      return sign == SIGNATURE;
    }

    const std::string FORMAT(
      "'T'F'M'D"
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::TFD_DECODER_DESCRIPTION;
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

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }
      try
      {
        Binary::InputStream stream(data);
        stream.ReadField<SignatureType>();
        target.SetTitle(FromStdString(stream.ReadCString(MAX_STRING_SIZE)));
        target.SetAuthor(FromStdString(stream.ReadCString(MAX_STRING_SIZE)));
        target.SetComment(FromStdString(stream.ReadCString(MAX_COMMENT_SIZE)));

        const std::size_t fixedOffset = stream.GetPosition();
        for (;;)
        {
          const uint8_t val = *stream.ReadData(1);
          if (val == FINISH)
          {
            break;
          }
          switch (val)
          {
          case BEGIN_FRAME:
            target.BeginFrames(1);
            break;
          case SKIP_FRAMES:
            target.BeginFrames(*stream.ReadData(1) + 3);
            break;
          case SELECT_SECOND_CHIP:
            target.SelectChip(1);
            break;
          case SELECT_FIRST_CHIP:
            target.SelectChip(0);
            break;
          case LOOP_MARKER:
            target.SetLoop();
            break;
          default:
            target.SetRegister(val, *stream.ReadData(1));
            break;
          }
        }
        const std::size_t usedSize = stream.GetPosition();
        const Binary::Container::Ptr subData = stream.GetReadData();
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
  }//namespace TFD

  Decoder::Ptr CreateTFDDecoder()
  {
    return MakePtr<TFD::Decoder>();
  }
}//namespace Chiptune
}//namespace Formats
