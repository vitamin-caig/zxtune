/*
Abstract:
  Playlist import for .ayl format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "import.h"
#include "container_impl.h"
#include "ui/utils.h"
#include "tags/ayl.h"
//common includes
#include <error.h>
#include <logging.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <devices/aym.h>
#include <sound/sound_parameters.h>
//std includes
#include <cctype>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>

namespace
{
  const std::string THIS_MODULE("Playlist::IO::AYL");

  /*
    Versions:
    0 -
    1 - PlayerFrequency parameter is in mHz
    2 -
    3 - \n tag in Comment field
    4 -
    5 -
    6 - UTF8 in all parameters field
  */
  class VersionLayer
  {
  public:
    explicit VersionLayer(int vers)
      : Version(vers)
      , Codec(QTextCodec::codecForName(Version > 5 ? "UTF-8" : "Windows-1251"))
    {
      assert(Codec);
    }

    String DecodeString(const std::string& str) const
    {
      return FromQString(Codec->toUnicode(str.c_str()));
    }

    Parameters::IntType DecodeFrameduration(Parameters::IntType playerFreq) const
    {
      if (!playerFreq)
      {
        return Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
      }
      const Parameters::IntType divisor = Version > 0
        ? UINT64_C(1000000000) : UINT64_C(1000000);
      return divisor / playerFreq;
    }
  private:
    const int Version;
    const QTextCodec* const Codec;
  };

  int CheckAYLBySignature(const String& signature)
  {
    const String strSignature(AYL::SIGNATURE);
    if (0 == signature.find(strSignature))
    {
      const Char versChar = signature[strSignature.size()];
      if (std::isdigit(versChar))
      {
        return versChar - '0';
      }
    }
    return -1;
  }

  class AYLContainer
  {
    struct AYLEntry
    {
      String Path;
      StringMap Parameters;
    };
    typedef std::vector<AYLEntry> AYLEntries;
    typedef RangeIterator<StringArray::const_iterator> LinesIterator;
  public:
    explicit AYLContainer(const StringArray& lines)
      : Container(boost::make_shared<AYLEntries>())
      , Parameters()
    {
      LinesIterator iter(lines.begin(), lines.end());
      //parse playlist parameters
      while (ParseParameters(iter, Parameters)) {}
      while (iter)
      {
        AYLEntry entry;
        entry.Path = *iter;
        ++iter;
        while (ParseParameters(iter, entry.Parameters)) {}
        std::replace(entry.Path.begin(), entry.Path.end(), '\\', '/');
        Container->push_back(entry);
      }
    }

    class Iterator
    {
    public:
      explicit Iterator(boost::shared_ptr<const AYLEntries> container)
        : Container(container)
        , Delegate(Container->begin(), Container->end())
      {
      }

      bool IsValid() const
      {
        return Delegate;
      }

      const String GetPath() const
      {
        return Delegate->Path;
      }

      const StringMap& GetParameters() const
      {
        return Delegate->Parameters;
      }

      void Next()
      {
        ++Delegate;
      }
    private:
      const boost::shared_ptr<const AYLEntries> Container;
      RangeIterator<AYLEntries::const_iterator> Delegate;
    };

    Iterator GetIterator() const
    {
      return Iterator(Container);
    }

    const StringMap& GetParameters() const
    {
      return Parameters;
    }
  private:
    static bool ParseParameters(LinesIterator& iter, StringMap& parameters)
    {
      if (!iter || !CheckForParametersBegin(*iter))
      {
        return false;
      }
      while (++iter)
      {
        const String line = *iter;
        if (CheckForParametersEnd(line))
        {
          ++iter;
          break;
        }
        SplitParametersString(line, parameters);
      }
      return true;
    }

    static bool CheckForParametersBegin(const String& line)
    {
      static const Char PARAMETERS_BEGIN[] = {'<', 0};
      return line == PARAMETERS_BEGIN;
    }

    static bool CheckForParametersEnd(const String& line)
    {
      static const Char PARAMETERS_END[] = {'>', 0};
      return line == PARAMETERS_END;
    }

    static void SplitParametersString(const String& line, StringMap& parameters)
    {
      const String::size_type delim = line.find_first_of('=');
      if (delim != String::npos)
      {
        const String& name = line.substr(0, delim);
        const String& value = line.substr(delim + 1);
        parameters.insert(StringMap::value_type(name, value));
      }
    }
  private:
    const boost::shared_ptr<AYLEntries> Container;
    StringMap Parameters;
  };

  class ParametersFilter : public Parameters::Visitor
  {
  public:
    ParametersFilter(const VersionLayer& version, Parameters::Visitor& delegate)
      : Version(version)
      , Delegate(delegate)
    {
    }

    virtual void SetIntValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      Log::Debug(THIS_MODULE, "  property %1%=%2%", name, val);
      if (name == AYL::CHIP_FREQUENCY)
      {
        Delegate.SetIntValue(Parameters::ZXTune::Sound::CLOCKRATE, val);
      }
      else if (name == AYL::PLAYER_FREQUENCY)
      {
        Delegate.SetIntValue(Parameters::ZXTune::Sound::FRAMEDURATION,
          Version.DecodeFrameduration(val));
      }
      //ignore "Loop", "Length", "Time"
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      Log::Debug(THIS_MODULE, "  property %1%='%2%'", name, val);
      if (name == AYL::CHIP_TYPE)
      {
        Delegate.SetIntValue(Parameters::ZXTune::Core::AYM::TYPE, DecodeChipType(val));
      }
      //ignore "Channels"
      else if (name == AYL::CHANNELS_ALLOCATION)
      {
        Delegate.SetIntValue(Parameters::ZXTune::Core::AYM::LAYOUT, DecodeChipLayout(val));
      }
      //ignore "Offset", "Length", "Address", "Loop", "Time", "Original"
      else if (name == AYL::NAME)
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_TITLE, Version.DecodeString(val));
      }
      else if (name == AYL::AUTHOR)
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_AUTHOR, Version.DecodeString(val));
      }
      else if (name == AYL::PROGRAM || name == AYL::TRACKER)
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_PROGRAM, Version.DecodeString(val));
      }
      else if (name == AYL::COMPUTER)
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_COMPUTER, Version.DecodeString(val));
      }
      else if (name == AYL::DATE)
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_DATE, Version.DecodeString(val));
      }
      else if (name == AYL::COMMENT)
      {
        //TODO: process escape sequence
        Delegate.SetStringValue(ZXTune::Module::ATTR_COMMENT, Version.DecodeString(val));
      }
      //ignore "Tracker", "Type", "ams_andsix", "FormatSpec"
    }

    virtual void SetDataValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      //try to process as string
      Delegate.SetStringValue(name, Parameters::ConvertToString(val));
    }
  private:
    static Parameters::IntType DecodeChipType(const String& value)
    {
      return value == AYL::YM ? 1 : 0;
    }

    static Parameters::IntType DecodeChipLayout(const String& value)
    {
      if (value == AYL::ACB)
      {
        return ZXTune::Module::AYM::LAYOUT_ACB;
      }
      else if (value == AYL::BAC)
      {
        return ZXTune::Module::AYM::LAYOUT_BAC;
      }
      else if (value == AYL::BCA)
      {
        return ZXTune::Module::AYM::LAYOUT_BCA;
      }
      else if (value == AYL::CAB)
      {
        return ZXTune::Module::AYM::LAYOUT_CAB;
      }
      else if (value == AYL::CBA)
      {
        return ZXTune::Module::AYM::LAYOUT_CBA;
      }
      else
      {
        //default fallback
        return ZXTune::Module::AYM::LAYOUT_ABC;
      }
    }
  private:
    const VersionLayer& Version;
    Parameters::Visitor& Delegate;
  };

  Parameters::Container::Ptr CreateProperties(const VersionLayer& version, const AYLContainer& aylItems)
  {
    const Parameters::Container::Ptr properties = Parameters::Container::Create();
    ParametersFilter filter(version, *properties);
    const StringMap& listParams = aylItems.GetParameters();
    Parameters::ParseStringMap(listParams, filter);
    return properties;
  }

  Playlist::IO::ContainerItemsPtr CreateItems(const QString& basePath, const VersionLayer& version, const AYLContainer& aylItems)
  {
    const QDir baseDir(basePath);
    const boost::shared_ptr<Playlist::IO::ContainerItems> items = boost::make_shared<Playlist::IO::ContainerItems>();
    for (AYLContainer::Iterator iter = aylItems.GetIterator(); iter.IsValid(); iter.Next())
    {
      const String& itemPath = iter.GetPath();
      Log::Debug(THIS_MODULE, "Processing '%1%'", itemPath);
      Playlist::IO::ContainerItem item;
      const QString absItemPath = baseDir.absoluteFilePath(ToQString(itemPath));
      item.Path = FromQString(baseDir.cleanPath(absItemPath));
      const Parameters::Container::Ptr adjustedParams = Parameters::Container::Create();
      ParametersFilter filter(version, *adjustedParams);
      const StringMap& itemParams = iter.GetParameters();
      Parameters::ParseStringMap(itemParams, filter);
      item.AdjustedParameters = adjustedParams;
      items->push_back(item);
    }
    return items;
  }

  bool CheckAYLByName(const QString& filename)
  {
    static const QString AYL_SUFFIX = QString::fromUtf8(AYL::SUFFIX);
    return filename.endsWith(AYL_SUFFIX, Qt::CaseInsensitive);
  }
}

namespace Playlist
{
  namespace IO
  {
    Container::Ptr OpenAYL(Item::DataProvider::Ptr provider, const QString& filename)
    {
      const QFileInfo info(filename);
      if (!info.isFile() || !info.isReadable() ||
          !CheckAYLByName(info.fileName()))
      {
        return Container::Ptr();
      }
      QFile device(filename);
      if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
      {
        assert(!"Failed to open playlist");
        return Container::Ptr();
      }
      QTextStream stream(&device);
      const String header = FromQString(stream.readLine(0).simplified());
      const int vers = CheckAYLBySignature(header);
      if (vers < 0)
      {
        return Container::Ptr();
      }
      Log::Debug(THIS_MODULE, "Processing AYL version %1%", vers);
      StringArray lines;
      while (!stream.atEnd())
      {
        const QString line = stream.readLine(0).trimmed();
        lines.push_back(FromQString(line));
      }
      const QString basePath = info.absolutePath();
      const VersionLayer version(vers);
      const AYLContainer aylItems(lines);
      const ContainerItemsPtr items = CreateItems(basePath, version, aylItems);
      const Parameters::Container::Ptr properties = CreateProperties(version, aylItems);
      properties->SetStringValue(Playlist::ATTRIBUTE_NAME, FromQString(info.baseName()));
      return Playlist::IO::CreateContainer(provider, properties, items);
    }
  }
}
