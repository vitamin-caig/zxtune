/**
* 
* @file
*
* @brief  RMT support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <byteorder.h>
#include <contract.h>
#include <pointers.h>
#include <make_ptr.h>
//library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/chiptune/container.h>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace RasterMusicTracker
  {
    //as for ASAP limitations
    //Details: http://atariki.krap.pl/index.php/RMT_%28format_pliku%29
    const StringView FORMAT =
        "00|ff 00|ff"   //signature
        "??"            //music address
        "??"            //initial data size
        "'R'M'T '4|'8" //signature
        "??"            //+a +b
        "01-04"         //tempo
        "01"            //+d
    ;
    
    const std::size_t MIN_SIZE = 0x30;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      String GetDescription() const override
      {
        return Text::RASTERMUSICTRACKER_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        try
        {
          Binary::InputStream stream(rawData);
          const auto sign = stream.ReadLE<uint16_t>();
          Require(sign == 0xffff || sign == 0x0000);
          const auto musicFirst = stream.ReadLE<uint16_t>();
          const auto musicLast = stream.ReadLE<uint16_t>();
          Require(musicFirst < musicLast);
          Require(musicFirst > 0xd800 || musicLast < 0xd000);
          const auto musicSize = musicLast + 1 - musicFirst;
          stream.Skip(musicSize);
          const auto fixedSize = stream.GetPosition();
          if (stream.GetRestSize())
          {
            const auto infoFirst = stream.ReadLE<uint16_t>();
            Require(infoFirst == musicLast + 1);
            const auto infoLast = stream.ReadLE<uint16_t>();
            const auto infoSize = infoLast + 1 - infoFirst;
            stream.Skip(infoSize);
          }
          return CreateCalculatingCrcContainer(stream.GetReadContainer(), 0, fixedSize);
        }
        catch (const std::exception&)
        {
        }
        return Formats::Chiptune::Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }

  Decoder::Ptr CreateRasterMusicTrackerDecoder()
  {
    return MakePtr<RasterMusicTracker::Decoder>();
  }
}
}
