/*
Abstract:
  Hobeta implicit convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <byteorder.h>
#include <tools.h>
#include <core/plugin_attrs.h>
#include <io/container.h>

#include <numeric>

#include <text/plugins.h>

#define FILE_TAG 1CF1A62A

namespace
{
  using namespace ZXTune;

  const Char HOBETA_PLUGIN_ID[] = {'H', 'o', 'b', 'e', 't', 'a', '\0'};
  const String TEXT_HOBETA_VERSION(FromStdString("$Rev$"));

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Header
  {
    uint8_t Filename[8];
    uint8_t Filetype[3];
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

  //////////////////////////////////////////////////////////////////////////
  bool ProcessHobeta(const Parameters::Map& /*commonParams*/, const MetaContainer& input,
    IO::DataContainer::Ptr& output, ModuleRegion& region)
  {
    const IO::DataContainer& inputData(*input.Data);
    const std::size_t limit(inputData.Size());
    const uint8_t* const data(static_cast<const uint8_t*>(inputData.Data()));
    const Header* const header(safe_ptr_cast<const Header*>(data));
    const std::size_t dataSize(fromLE(header->Length));
    const std::size_t fullSize(fromLE(header->FullLength));
    if (dataSize < HOBETA_MIN_SIZE ||
        dataSize > HOBETA_MAX_SIZE ||
        dataSize + sizeof(*header) > limit ||
        fullSize != align<std::size_t>(dataSize, 256) ||
        header->Filetype + 1 != std::find_if(header->Filename, header->Filetype + 1, 
          std::bind2nd(std::less<uint8_t>(), uint8_t(' ')))
        )
    {
      return false;
    }
    if (fromLE(header->CRC) == ((105 + 257 * std::accumulate(data, data + 15, 0u)) & 0xffff))
    {
      region.Offset = 0;
      region.Size = fullSize + sizeof(*header);
      output = inputData.GetSubcontainer(sizeof(*header), dataSize);
      return true;
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterHobetaConvertor(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    info.Id = HOBETA_PLUGIN_ID;
    info.Description = TEXT_HOBETA_INFO;
    info.Version = TEXT_HOBETA_VERSION;
    info.Capabilities = CAP_STOR_CONTAINER | CAP_STOR_PLAIN;
    enumerator.RegisterImplicitPlugin(info, ProcessHobeta);
  }
}
