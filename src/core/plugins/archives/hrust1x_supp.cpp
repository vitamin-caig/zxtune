/*
Abstract:
  Hrust 1.x convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/detect_helper.h>
#include <core/plugins/registrator.h>
//common includes
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <formats/packed_decoders.h>
#include <io/container.h>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 047CB563

namespace
{
  using namespace ZXTune;

  const Char HRUST1X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '1', '\0'};
  const String HRUST1X_PLUGIN_VERSION(FromStdString("$Rev$"));

  //checkers
  const DataPrefixChecker DEPACKERS[] =
  {
    DataPrefixChecker
    (
      "f3"      // di
      "ed73??"  // ld (xxxx),sp
      "11??"    // ld de,xxxx
      "21??"    // ld hl,xxxx
      "01??"    // ld bc,xxxx
      "d5"      // push de
      "edb0"    // ldir
      "13"      // inc de
      "13"      // inc de
      "d5"      // push de
      "dde1"    // pop ix
      "0e?"     // ld c,xx
      "09"      // add hl,bc
      "edb0"    // ldir
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx
      "c9"      // ret
      ,
      0x103
    ),

    DataPrefixChecker
    (
      "dd21??"  // ld ix,xxxx
      "dd39"    // add ix,sp
      "d5"      // push de
      "f9"      // ld sp,hl
      "c1"      // pop bc
      "eb"      // ex de,hl
      "c1"      // pop bc
      "0b"      // dec bc
      "09"      // add hl,bc
      "eb"      // ex de,hl
      "c1"      // pop bc
      "0b"      // dec bc
      "09"      // add hl,bc
      "ed52"    // sbc hl,de
      "19"      // add hl,de
      "38?"     // jr c,...
      "54"      // ld d,h
      "5d"      // ld e,l
      "edb8"    // lddr
      "eb"      // ex de,hl
      "dd560b"  // ld d,(ix+0xb)
      "dd5e0a"  // ld e,(ix+0xa)
      "f9"      // ld sp,hl
      "e1"      // pop hl
      "e1"      // pop hl
      "e1"      // pop hl
      "06?"     // ld b,xx (6)
      "3b"      // dec sp
      "f1"      // pop af
      "dd7706"  // ld (ix+6),a
      "dd23"    // inc ix
      "10?"     // djnz xxx, (0xf7)
      ,
      0x100
    )
  };

  class Hrust1xPlugin : public ArchivePlugin
                      , private ArchiveDetector
  {
  public:
    Hrust1xPlugin()
      : Decoder(Formats::Packed::CreateHrust1Decoder())
    {
    }

    virtual String Id() const
    {
      return HRUST1X_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUST1X_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUST1X_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckDataFormat(*this, inputData);
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& parameters,
      const IO::DataContainer& input, std::size_t& usedSize) const
    {
      return ExtractSubdataFromData(*this, parameters, input, usedSize);
    }
  private:
    virtual bool CheckData(const uint8_t* data, std::size_t size) const
    {
      return Decoder->Check(data, size);
    }

    virtual DataPrefixIterator GetPrefixes() const
    {
      return DataPrefixIterator(DEPACKERS, ArrayEnd(DEPACKERS));
    }

    virtual IO::DataContainer::Ptr TryToExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const IO::DataContainer& data, std::size_t& packedSize) const
    {
      std::auto_ptr<Dump> res = Decoder->Decode(data.Data(), data.Size(), packedSize);
      if (res.get())
      {
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  private:
    const Formats::Packed::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterHrust1xConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new Hrust1xPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
