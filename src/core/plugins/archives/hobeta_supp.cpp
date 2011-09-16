/*
Abstract:
  Hobeta convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "archive_supp_common.h"
#include "core/plugins/registrator.h"
#include "formats/packed/container.h"
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed.h>
#include <io/container.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Header
  {
    uint8_t Filename[9];
    uint16_t Start;
    uint16_t Length;
    uint16_t FullLength;
    uint16_t CRC;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 17);
  const std::size_t HOBETA_MIN_SIZE = 0x100;
  const std::size_t HOBETA_MAX_SIZE = 0xff00;

  bool CheckHobeta(const void* rawData, std::size_t limit)
  {
    if (limit < sizeof(Header))
    {
      return false;
    }
    const uint8_t* const data = static_cast<const uint8_t*>(rawData);
    const Header* const header = static_cast<const Header*>(rawData);
    const std::size_t dataSize = fromLE(header->Length);
    const std::size_t fullSize = fromLE(header->FullLength);
    if (dataSize < HOBETA_MIN_SIZE ||
        dataSize > HOBETA_MAX_SIZE ||
        dataSize + sizeof(*header) > limit ||
        fullSize != align<std::size_t>(dataSize, 256) ||
        //check for valid name
        ArrayEnd(header->Filename) != std::find_if(header->Filename, ArrayEnd(header->Filename),
          std::bind2nd(std::less<uint8_t>(), uint8_t(' ')))
        )
    {
      return false;
    }
    //check for crc
    if (fromLE(header->CRC) == ((105 + 257 * std::accumulate(data, data + 15, 0u)) & 0xffff))
    {
      return true;
    }
    return false;
  }

  const std::string HOBETA_FORMAT(
    //Filename
    "20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a"
    //Start
    "??"
    //Length
    "?01-ff"
    //FullLength
    "0001-ff"
  );

  class HobetaDecoder : public Formats::Packed::Decoder
  {
  public:
    HobetaDecoder()
      : Format(Binary::Format::Create(HOBETA_FORMAT))
    {
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual bool Check(const void* data, std::size_t availSize) const
    {
      return Format->Match(data, availSize) && CheckHobeta(data, availSize);
    }

    virtual Formats::Packed::Container::Ptr Decode(const void* rawData, std::size_t availSize) const
    {
      if (!CheckHobeta(rawData, availSize))
      {
        return Formats::Packed::Container::Ptr();
      }
      const uint8_t* const data = static_cast<const uint8_t*>(rawData);
      const Header* const header = safe_ptr_cast<const Header*>(rawData);
      const std::size_t dataSize = fromLE(header->Length);
      const std::size_t fullSize = fromLE(header->FullLength);
      return CreatePackedContainer(std::auto_ptr<Dump>(new Dump(data + sizeof(*header), data + sizeof(*header) + dataSize)), fullSize + sizeof(*header));
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace
{
  using namespace ZXTune;

  const Char ID[] = {'H', 'O', 'B', 'E', 'T', 'A', '\0'};
  const String VERSION(FromStdString("$Rev$"));
  const Char* const INFO = Text::HOBETA_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_CONTAINER | CAP_STOR_PLAIN;
}

namespace ZXTune
{
  void RegisterHobetaConvertor(PluginsRegistrator& registrator)
  {
    Formats::Packed::Decoder::Ptr decoder(new HobetaDecoder());
    const ArchivePlugin::Ptr plugin = CreateArchivePlugin(ID, INFO, VERSION, CAPS, decoder);
    registrator.RegisterPlugin(plugin);
  }
}
