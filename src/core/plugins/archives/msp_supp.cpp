/*
Abstract:
  MsPack convertors support

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

namespace
{
  using namespace ZXTune;

  const Char MSP_PLUGIN_ID[] = {'M', 'S', 'P', '\0'};
  const String MSP_PLUGIN_VERSION(FromStdString("$Rev$"));

  //checkers
  const DataPrefixChecker DEPACKERS[] =
  {
    DataPrefixChecker
    (
      "?"       // di/nop
      "ed73??"  // ld (xxxx),sp
      "d9"      // exx
      "22??"    // ld (xxxx),hl
      "0e?"     // ld c,xx
      "41"      // ld b,c
      "16?"     // ld d,xx
      "d9"      // exx
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx
      "edb0"    // ldir
      "f9"      // ld sp,hl
      "e1"      // pop hl
      "0e?"     // ld c,xx
      "edb8"    // lddr
      "e1"      // pop hl
      "d1"      // pop de
      "c1"      // pop bc
      "31??"    // ld sp,xxxx
      "c3??"    // jp xxxx
      ,
      0xe5
    )
  };

  class MSPPlugin : public ArchivePlugin
                  , private ArchiveDetector
  {
  public:
    MSPPlugin()
      : Decoder(Formats::Packed::CreateMSPackDecoder())
      , Format(Decoder->GetFormat())
    {
    }

    virtual String Id() const
    {
      return MSP_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::MSP_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return MSP_PLUGIN_VERSION;
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
      const IO::DataContainer& data, std::size_t& usedSize) const
    {
      return ExtractSubdataFromData(*this, parameters, data, usedSize);
    }
  private:
    virtual bool CheckData(const uint8_t* data, std::size_t size) const
    {
      return Format->Match(data, size);
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
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterMSPConvertor(PluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin(new MSPPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
