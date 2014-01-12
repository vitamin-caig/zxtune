//local includes
#include "core/plugins/plugins_types.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
//common includes
#include <contract.h>
#include <byteorder.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/archived.h>

namespace
{
  const Debug::Stream Dbg("Core::ArchivesSupp");
}

namespace ZXTune
{
namespace Sid
{
  //TODO: extract to Formats::Archived library

  const Char ID[] = {'S', 'I', 'D', 0};
  const uint_t CAPS = CAP_STOR_MULTITRACK;

  const std::string FORMAT =
      "'R|'P 'S'I'D" //signature
      "00 01|02"     //BE version
      "00 76|7c"     //BE data offset
      "??"           //BE load address
      "??"           //BE init address
      "??"           //BE play address
      "00|01 ?"      //BE songs count 1-256
      "??"           //BE start song
      "????"         //BE speed flag
   ;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    uint8_t Signature[4];
    uint16_t Version;
    uint16_t DataOffset;
    uint16_t LoadAddr;
    uint16_t InitAddr;
    uint16_t PlayAddr;
    uint16_t SoungsCount;
    uint16_t StartSong;
    uint32_t SpeedFlags;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const IndexPathComponent SUBPATH_FORMAT(Text::SID_FILENAME_PREFIX);

  /*
   * Since md5 digest of sid modules depends on tracks count, it's impossible to fix it
   * (at least without length database renewal). This container plugin can be applied only once while search
   * and only for multitrack modules.
   */
  class ContainerPlugin : public ArchivePlugin
  {
  public:
    ContainerPlugin()
      : Description(CreatePluginDescription(ID, Text::SID_ARCHIVE_DECODER_DESCRIPTION, CAPS))
      , Format(Binary::CreateMatchOnlyFormat(FORMAT))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr rawData = inputData->GetData();
      try
      {
        const uint_t songs = GetSongsCount(rawData, inputData->GetPluginsChain());
        Require(songs > 1);
        Dbg("Processing SID container with %1% tunes", songs);
        for (uint_t sng = 1; sng <= songs; ++sng)
        {
          const Binary::Container::Ptr subData = CreateFixedModule(rawData, sng);
          const DataLocation::Ptr subLocation = CreateNestedLocation(inputData, subData, Description->Id(), SUBPATH_FORMAT.Build(sng));
          Module::Detect(subLocation, callback);
        }
        return Analysis::CreateMatchedResult(rawData->Size());
      }
      catch (const std::exception&)
      {
        return Analysis::CreateUnmatchedResult(rawData->Size());
      }
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& parameters,
                                   DataLocation::Ptr inputData,
                                   const Analysis::Path& pathToOpen) const
    {
      try
      {
        const Binary::Container::Ptr rawData = inputData->GetData();
        const uint_t songs = GetSongsCount(rawData, inputData->GetPluginsChain());
        Require(songs > 1);

        const String subPath = pathToOpen.AsString();
        uint_t song = 0;
        Require(SUBPATH_FORMAT.GetIndex(subPath, song));
        Require(song <= songs);
        const Binary::Container::Ptr subData = CreateFixedModule(rawData, song);
        return CreateNestedLocation(inputData, subData, Description->Id(), subPath);
      }
      catch (const std::exception&)
      {
        return DataLocation::Ptr();
      }
    }
  private:
    uint_t GetSongsCount(Binary::Container::Ptr rawData, Analysis::Path::Ptr plugs) const
    {
      if (!Format->Match(*rawData) || HasPathElement(*plugs, Description->Id()))
      {
        return 0;
      }
      const RawHeader& hdr = *safe_ptr_cast<const RawHeader*>(rawData->Start());
      return fromBE(hdr.SoungsCount);
    }

    static bool HasPathElement(const Analysis::Path& path, const String& id)
    {
      for (Analysis::Path::Iterator::Ptr it = path.GetIterator(); it->IsValid(); it->Next())
      {
        if (it->Get() == id)
        {
          return true;
        }
      }
      return false;
    }

    static Binary::Container::Ptr CreateFixedModule(Binary::Data::Ptr rawData, uint_t sng)
    {
      std::auto_ptr<Dump> content(new Dump(rawData->Size()));
      std::memcpy(&content->front(), rawData->Start(), content->size());
      RawHeader& hdr = *safe_ptr_cast<RawHeader*>(&content->front());
      hdr.StartSong = fromBE<uint16_t>(sng);
      return Binary::CreateContainer(content);
    }
  private:
    const Plugin::Ptr Description;
    const Binary::Format::Ptr Format;
  };
}
}

namespace ZXTune
{
  void RegisterSidContainer(ArchivePluginsRegistrator& registrator)
  {
    const ArchivePlugin::Ptr plugin = boost::make_shared<Sid::ContainerPlugin>();
    registrator.RegisterPlugin(plugin);
  }
}
