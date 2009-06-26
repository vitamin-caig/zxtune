#include "player_base.h"

#include <error.h>

#include <convert_parameters.h>

#include <boost/crc.hpp>

#include <text/errors.h>

#define FILE_TAG BBFD2A96

using namespace ZXTune;

PlayerBase::PlayerBase(const String& filename)
{
  Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
}

void PlayerBase::GetModuleInfo(Module::Information& info) const
{
  info = Information;
}

void PlayerBase::Convert(const Conversion::Parameter& param, Dump& dst) const
{
  using namespace Conversion;
  if (const RawConvertParam* const p = parameter_cast<RawConvertParam>(&param))
  {
    dst = RawData;
  }
  else
  {
    throw Error(ERROR_DETAIL, 1, TEXT_ERROR_CONVERSION_UNSUPPORTED);//TODO: code
  }
}

void PlayerBase::FillProperties(const String& tracker, const String& author, const String& title, 
      const uint8_t* data, std::size_t size)
{
  if (!tracker.empty() && String::npos != tracker.find_first_not_of(' '))
  {
    Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, tracker));
  }
  if (!author.empty() && String::npos != author.find_first_not_of(' '))
  {
    Information.Properties.insert(StringMap::value_type(Module::ATTR_AUTHOR, author));
  }
  if (!title.empty() && String::npos != title.find_first_not_of(' '))
  {
    Information.Properties.insert(StringMap::value_type(Module::ATTR_TITLE, title));
  }
  RawData.assign(data, data + size);
  boost::crc_32_type crcCalc;
  crcCalc.process_bytes(&RawData[0], size);
  Information.Properties.insert(StringMap::value_type(Module::ATTR_CRC, 
    string_cast(std::hex, crcCalc.checksum())));
}
