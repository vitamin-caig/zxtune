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
#include "ui/utils.h"
//common includes
#include <error.h>
#include <logging.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <devices/aym.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/variant/get.hpp>
//qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QTextStream>

namespace
{
  const std::string THIS_MODULE("UI::PlaylistAYL");

  String ConcatenatePath(const String& baseDirPath, const String& subPath)
  {
    const QDir baseDir(ToQString(baseDirPath));
    const QFileInfo file(baseDir, ToQString(subPath));
    return FromQString(file.canonicalFilePath());
  }

  int CheckAYLBySignature(const String& signature)
  {
    static const Char AYL_SIGNATURE[] = 
    {
     'Z','X',' ','S','p','e','c','t','r','u','m',' ',
     'S','o','u','n','d',' ','C','h','i','p',' ','E','m','u','l','a','t','o','r',' ',
     'P','l','a','y',' ','L','i','s','t',' ','F','i','l','e',' ','v','1','.',
     0
    };
    const String strSignature(AYL_SIGNATURE);
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

  class AYLIterator
  {
    struct AYLEntry
    {
      String Path;
      StringMap Parameters;
    };
    typedef std::vector<AYLEntry> AYLEntries;
    typedef RangeIterator<StringArray::const_iterator> LinesIterator;
  public:
    explicit AYLIterator(const StringArray& lines)
    {
      LinesIterator iter(lines.begin(), lines.end());
      while (iter)
      {
        AYLEntry entry;
        entry.Path = *iter;
        ++iter;
        ParseParameters(iter, entry.Parameters);
        Container.push_back(entry);
      }
      Current = Container.begin();
    }

    bool IsValid() const
    {
      return Current != Container.end();
    }

    const String GetPath() const
    {
      return Current->Path;
    }

    const StringMap& GetParameters() const
    {
      return Current->Parameters;
    }

    void Next()
    {
      ++Current;
    }
  private:
    static void ParseParameters(LinesIterator& iter, StringMap& parameters)
    {
      if (!iter || !CheckForParametersBegin(*iter))
      {
        return;
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
    AYLEntries Container;
    AYLEntries::const_iterator Current;
  };

  class CollectorStub : public PlayitemDetectParameters
  {
  public:
    virtual bool ProcessPlayitem(Playitem::Ptr item)
    {
      if (Item)
      {
        //do not support more than one module in container
        return false;
      }
      Item = item;
      return true;
    }

    virtual void ShowProgress(const Log::MessageData& /*msg*/)
    {
    }

    Playitem::Ptr GetItem() const
    {
      return Item;
    }
  private:
    Playitem::Ptr Item;
  };


  Parameters::IntType DecodeChipType(const String& value)
  {
    return value == "YM" ? 1 : 0;
  }

  Parameters::IntType DecodeChipLayout(const String& value)
  {
    if (value == "ACB")
    {
      return ZXTune::AYM::LAYOUT_ACB;
    }
    else if (value == "BAC")
    {
      return ZXTune::AYM::LAYOUT_BAC;
    }
    else if (value == "BCA")
    {
      return ZXTune::AYM::LAYOUT_BCA;
    }
    else if (value == "CAB")
    {
      return ZXTune::AYM::LAYOUT_CAB;
    }
    else if (value == "CBA")
    {
      return ZXTune::AYM::LAYOUT_CBA;
    }
    else
    {
      //default fallback
      return ZXTune::AYM::LAYOUT_ABC;
    }
  }

  Parameters::IntType DecodeClockrate(const String& value)
  {
    const Parameters::ValueType val = Parameters::ConvertFromString(value);
    if (const Parameters::IntType* asInt = boost::get<const Parameters::IntType>(&val))
    {
      return *asInt;
    }
    return Parameters::ZXTune::Sound::CLOCKRATE_DEFAULT;
  }

  Parameters::IntType DecodeFrameduration(const String& value)
  {
    const Parameters::ValueType val = Parameters::ConvertFromString(value);
    if (const Parameters::IntType* asInt = boost::get<const Parameters::IntType>(&val))
    {
      if (*asInt)
      {
        //TODO: divisor is 1M for playlist ver 0
        const Parameters::IntType divisor = UINT64_C(1000000000);
        return divisor / *asInt;
      }
    }
    return Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
  }

  void DecodeParameter(const String& name, const String& value, Parameters::Container& container)
  {
    Log::Debug(THIS_MODULE, "  Decode %1%='%2%'", name, value);
    if (name == "ChipType")
    {
      container.SetIntValue(Parameters::ZXTune::Core::AYM::TYPE, DecodeChipType(value));
    }
    //ignore "Channels"
    else if (name == "ChannelsAllocation")
    {
      container.SetIntValue(Parameters::ZXTune::Core::AYM::LAYOUT, DecodeChipLayout(value));
    }
    else if (name == "ChipFrequency")
    {
      container.SetIntValue(Parameters::ZXTune::Sound::CLOCKRATE, DecodeClockrate(value));
    }
    else if (name == "PlayerFrequency")
    {
      container.SetIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, DecodeFrameduration(value));
    }
    //ignore "Offset", "Length", "Address", "Loop", "Time", "Original"
    else if (name == "Name")
    {
      container.SetStringValue(ZXTune::Module::ATTR_TITLE, value);
    }
    else if (name == "Author")
    {
      container.SetStringValue(ZXTune::Module::ATTR_AUTHOR, value);
    }
    else if (name == "Program" || name == "Tracker")
    {
      container.SetStringValue(ZXTune::Module::ATTR_PROGRAM, value);
    }
    else if (name == "Computer")
    {
      container.SetStringValue(ZXTune::Module::ATTR_COMPUTER, value);
    }
    else if (name == "Date")
    {
      container.SetStringValue(ZXTune::Module::ATTR_DATE, value);
    }
    else if (name == "Comment")
    {
      container.SetStringValue(ZXTune::Module::ATTR_COMMENT, value);
    }
    //ignore "Tracker", "Type", "ams_andsix", "FormatSpec"
  }

  Playitem::Ptr OpenPlayitem(const PlayitemsProvider& provider, const String& path, const StringMap& parameters)
  {
    CollectorStub collector;
    //error is ignored, just take from collector
    provider.DetectModules(path, collector);
    if (const Playitem::Ptr item = collector.GetItem())
    {
      Log::Debug(THIS_MODULE, "Opened '%1%'", path);
      const Parameters::Container::Ptr params = item->GetAdjustedParameters();
      std::for_each(parameters.begin(), parameters.end(),
        boost::bind(&DecodeParameter, 
          boost::bind(&StringMap::value_type::first, _1),
          boost::bind(&StringMap::value_type::second, _1),
          boost::ref(*params)));
      return item;
    }
    Log::Debug(THIS_MODULE, "Failed to open '%1%'", path);
    return Playitem::Ptr();
  }

  class PlayitemsIterator : public Playitem::Iterator
  {
  public:
    PlayitemsIterator(PlayitemsProvider::Ptr provider, const String& basePath, const StringArray& lines)
      : Provider(provider)
      , BasePath(basePath)
      , Subiterator(lines)
    {
      FetchItem();
    }

    virtual bool IsValid() const
    {
      return Subiterator.IsValid();
    }

    virtual Playitem::Ptr Get() const
    {
      return Item;
    }

    virtual void Next()
    {
      Subiterator.Next();
      FetchItem();
    }
  private:
    void FetchItem()
    {
      for (; Subiterator.IsValid(); Subiterator.Next())
      {
        const String& itemPath = Subiterator.GetPath();
        const String path = ConcatenatePath(BasePath, itemPath);
        if (Item = OpenPlayitem(*Provider, path, Subiterator.GetParameters()))
        {
          return;
        }
      }
      Item.reset(); 
    }
  private:
    const PlayitemsProvider::Ptr Provider;
    const String BasePath;
    AYLIterator Subiterator;
    Playitem::Ptr Item;
  };


  bool CheckAYLByName(const QString& filename)
  {
    static const QString AYL_SUFFIX = QString::fromUtf8(".ayl");
    return filename.endsWith(AYL_SUFFIX, Qt::CaseInsensitive);
  }
}

Playitem::Iterator::Ptr OpenAYLPlaylist(PlayitemsProvider::Ptr provider, const QString& filename)
{
  Log::Debug(THIS_MODULE, "Trying to open '%1%' as a playlist", FromQString(filename));
  const QFileInfo info(filename);
  if (!info.isFile() || !info.isReadable() ||
      !CheckAYLByName(info.fileName()))
  {
    return Playitem::Iterator::Ptr();
  }
  QFile device(filename);
  if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    assert(!"Failed to open playlist");
    return Playitem::Iterator::Ptr();
  }
  QTextStream stream(&device);
  const String header = FromQString(stream.readLine(0).simplified());
  const int version = CheckAYLBySignature(header);
  if (version < 0)
  {
    return Playitem::Iterator::Ptr();
  }
  Log::Debug(THIS_MODULE, "Processing AYL version %1%", version);
  StringArray lines;
  while (!stream.atEnd())
  {
    const QString line = stream.readLine(0).simplified();
    lines.push_back(FromQString(line));
  }
  const String basePath = FromQString(info.absolutePath());
  return Playitem::Iterator::Ptr(new PlayitemsIterator(provider, basePath, lines));
}

Playitem::Iterator::Ptr OpenPlaylist(PlayitemsProvider::Ptr provider, const QString& filename)
{
  return OpenAYLPlaylist(provider, filename);
}