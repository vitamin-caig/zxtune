#pragma once

#include "module/players/factory.h"

#include "formats/chiptune/decoder.h"

namespace Module::Xmp
{
  ExternalParsingFactory::Ptr CreateDTTFactory();
  ExternalParsingFactory::Ptr CreateEMODFactory();
  ExternalParsingFactory::Ptr CreateFNKFactory();
  ExternalParsingFactory::Ptr CreateLIQFactory();
  ExternalParsingFactory::Ptr CreateMED2Factory();
  ExternalParsingFactory::Ptr CreateMED3Factory();
  ExternalParsingFactory::Ptr CreateMED4Factory();
  ExternalParsingFactory::Ptr CreateNOFactory();
  ExternalParsingFactory::Ptr CreateSTIMFactory();
  ExternalParsingFactory::Ptr CreateSTXFactory();
}  // namespace Module::Xmp

namespace Formats::Chiptune
{
  Decoder::Ptr CreateDTTDecoder();
  Decoder::Ptr CreateEMODDecoder();
  Decoder::Ptr CreateFNKDecoder();
  Decoder::Ptr CreateLIQDecoder();
  Decoder::Ptr CreateMED2Decoder();
  Decoder::Ptr CreateMED3Decoder();
  Decoder::Ptr CreateMED4Decoder();
  Decoder::Ptr CreateNODecoder();
  Decoder::Ptr CreateSTIMDecoder();
  Decoder::Ptr CreateSTXDecoder();
}  // namespace Formats::Chiptune
