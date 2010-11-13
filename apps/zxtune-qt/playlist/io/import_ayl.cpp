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
//std includes
#include <cctype>
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

  String FromUtf8String(const std::string& str)
  {
    const QString utf8(QString::fromUtf8(str.c_str()));
    return FromQString(utf8);
  }

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
    {
    }

    String DecodeString(const std::string& str) const
    {
      return Version > 5
        ? FromUtf8String(str)
        : FromStdString(str);
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
  };

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

  class ParametersFilter : public Parameters::Modifier
  {
  public:
    ParametersFilter(const VersionLayer& version, Parameters::Modifier& delegate)
      : Version(version)
      , Delegate(delegate)
    {
    }

    virtual void SetIntValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      Log::Debug(THIS_MODULE, "  property %1%=%2%", name, val);
      if (name == "ChipFrequency")
      {
        Delegate.SetIntValue(Parameters::ZXTune::Sound::CLOCKRATE, val);
      }
      else if (name == "PlayerFrequency")
      {
        Delegate.SetIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, 
          Version.DecodeFrameduration(val));
      }
      else
      {
        //try to process as string
        Delegate.SetStringValue(name, Parameters::ConvertToString(val));
      }
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      Log::Debug(THIS_MODULE, "  property %1%='%2%'", name, val);
      if (name == "ChipType")
      {
        Delegate.SetIntValue(Parameters::ZXTune::Core::AYM::TYPE, DecodeChipType(val));
      }
      //ignore "Channels"
      else if (name == "ChannelsAllocation")
      {
        Delegate.SetIntValue(Parameters::ZXTune::Core::AYM::LAYOUT, DecodeChipLayout(val));
      }
      //ignore "Offset", "Length", "Address", "Loop", "Time", "Original"
      else if (name == "Name")
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_TITLE, Version.DecodeString(val));
      }
      else if (name == "Author")
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_AUTHOR, Version.DecodeString(val));
      }
      else if (name == "Program" || name == "Tracker")
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_PROGRAM, Version.DecodeString(val));
      }
      else if (name == "Computer")
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_COMPUTER, Version.DecodeString(val));
      }
      else if (name == "Date")
      {
        Delegate.SetStringValue(ZXTune::Module::ATTR_DATE, Version.DecodeString(val));
      }
      else if (name == "Comment")
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
      return value == "YM" ? 1 : 0;
    }

    static Parameters::IntType DecodeChipLayout(const String& value)
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
  private:
    const VersionLayer& Version;
    Parameters::Modifier& Delegate;
  };

  class PlayitemAccessor
  {
  public:
    PlayitemAccessor(PlayitemsProvider::Ptr provider, const String& basePath, int vers)
      : Provider(provider)
      , BasePath(basePath)
      , Version(vers)
    {
    }

    Playitem::Ptr GetItem(const AYLIterator& iterator) const
    {
      const String& itemPath = iterator.GetPath();
      const String path = ConcatenatePath(BasePath, itemPath);
      return OpenPlayitem(path, iterator.GetParameters());
    }
  private:
    Playitem::Ptr OpenPlayitem(const String& path, const StringMap& parameters) const
    {
      CollectorStub collector;
      //error is ignored, just take from collector
      Provider->DetectModules(path, collector);
      if (const Playitem::Ptr item = collector.GetItem())
      {
        Log::Debug(THIS_MODULE, "Opened '%1%'", path);
        const Parameters::Container::Ptr params = item->GetAdjustedParameters();
        ParametersFilter filter(Version, *params);
        Parameters::ParseStringMap(parameters, filter);
        return item;
      }
      Log::Debug(THIS_MODULE, "Failed to open '%1%'", path);
      return Playitem::Ptr();
    }
  private:
    const PlayitemsProvider::Ptr Provider;
    const String BasePath;
    const VersionLayer Version;
  };

  class PlayitemsIterator : public Playitem::Iterator
  {
  public:
    PlayitemsIterator(PlayitemsProvider::Ptr provider, const String& basePath, int vers, const StringArray& lines)
      : Accessor(provider, basePath, vers)
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
      Item.reset(); 
      for (; Subiterator.IsValid(); Subiterator.Next())
      {
        if (Item = Accessor.GetItem(Subiterator))
        {
          return;
        }
      }
    }
  private:
    PlayitemAccessor Accessor;
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
  const int vers = CheckAYLBySignature(header);
  if (vers < 0)
  {
    return Playitem::Iterator::Ptr();
  }
  Log::Debug(THIS_MODULE, "Processing AYL version %1%", vers);
  StringArray lines;
  while (!stream.atEnd())
  {
    const QString line = stream.readLine(0).simplified();
    lines.push_back(FromQString(line));
  }
  const String basePath = FromQString(info.absolutePath());
  return Playitem::Iterator::Ptr(new PlayitemsIterator(provider, basePath, vers, lines));
}

Playitem::Iterator::Ptr OpenPlaylist(PlayitemsProvider::Ptr provider, const QString& filename)
{
  return OpenAYLPlaylist(provider, filename);
}
