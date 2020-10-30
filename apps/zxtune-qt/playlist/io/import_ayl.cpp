/**
* 
* @file
*
* @brief Import .ayl implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "import.h"
#include "container_impl.h"
#include "ui/utils.h"
#include "tags/ayl.h"
//common includes
#include <error.h>
#include <make_ptr.h>
//library includes
#include <core/core_parameters.h>
#include <core/plugins/utils.h>
#include <debug/log.h>
#include <devices/aym/chip.h>
#include <io/api.h>
#include <module/attributes.h>
#include <parameters/convert.h>
#include <parameters/serialize.h>
#include <sound/sound_parameters.h>
#include <strings/prefixed_index.h>
//std includes
#include <cctype>
//boost includes
#include <boost/algorithm/string/predicate.hpp>
//qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
//text includes
#include <core/text/plugins.h>
#include <formats/text/archived.h>

namespace
{
  const Debug::Stream Dbg("Playlist::IO::AYL");

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

    String DecodeString(const QString& str) const
    {
      return FromQString(Codec->toUnicode(str.toLocal8Bit()));
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

  class LinesSource
  {
  public:
    LinesSource(QTextStream& stream, const VersionLayer& version)
      : Stream(stream)
      , Version(version)
      , Valid(true)
    {
      Next();
    }

    bool IsValid() const
    {
      return Valid;
    }

    void Next()
    {
      if ((Valid = !Stream.atEnd()))
      {
        Line = Version.DecodeString(Stream.readLine(0).trimmed());
      }
      else
      {
        Line.clear();
      }
    }

    String GetLine() const 
    {
      return Line;
    }

    uint_t GetSize() const
    {
      return Stream.device()->size();
    }

    uint_t GetPosition()
    {
      return Stream.pos();
    }
  private:
    QTextStream& Stream;
    const VersionLayer& Version;
    String Line;
    bool Valid;
  };

  class AYLContainer
  {
    struct AYLEntry
    {
      String Path;
      Strings::Map Parameters;
    };
    struct AYLEntries : public std::vector<AYLEntry>
    {
      typedef std::shared_ptr<const AYLEntries> Ptr;
      typedef std::shared_ptr<AYLEntries> RWPtr;
    };
  public:
    AYLContainer(LinesSource& source, Log::ProgressCallback& cb)
      : Container(MakeRWPtr<AYLEntries>())
      , Parameters()
    {
      const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(source.GetSize(), cb);
      const uint_t REPORT_PERIOD_ITEMS = 1000;
      //parse playlist parameters
      while (ParseParameters(source, Parameters)) {}
      for (uint_t counter = 0; source.IsValid(); ++counter)
      {
        ParseEntry(source);
        if (++counter >= REPORT_PERIOD_ITEMS)
        {
          progress->OnProgress(source.GetPosition());
          counter = 0;
        }
      }
    }

    class Iterator
    {
    public:
      explicit Iterator(AYLEntries::Ptr container)
        : Container(std::move(container))
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

      const Strings::Map& GetParameters() const
      {
        return Delegate->Parameters;
      }

      void Next()
      {
        ++Delegate;
      }
    private:
      const AYLEntries::Ptr Container;
      RangeIterator<AYLEntries::const_iterator> Delegate;
    };

    Iterator GetIterator() const
    {
      return Iterator(Container);
    }

    const Strings::Map& GetParameters() const
    {
      return Parameters;
    }
  private:
    void ParseEntry(LinesSource& source)
    {
      AYLEntry entry;
      entry.Path = source.GetLine();
      source.Next();
      while (ParseParameters(source, entry.Parameters)) {}
      std::replace(entry.Path.begin(), entry.Path.end(), '\\', '/');
      Container->push_back(entry);
    }

    static bool ParseParameters(LinesSource& source, Strings::Map& parameters)
    {
      if (!source.IsValid() || !CheckForParametersBegin(source.GetLine()))
      {
        return false;
      }
      while (source.Next(), source.IsValid())
      {
        if (CheckForParametersEnd(source.GetLine()))
        {
          source.Next();
          break;
        }
        SplitParametersString(source.GetLine(), parameters);
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

    static void SplitParametersString(const String& line, Strings::Map& parameters)
    {
      const String::size_type delim = line.find_first_of('=');
      if (delim != String::npos)
      {
        const String& name = line.substr(0, delim);
        const String& value = line.substr(delim + 1);
        parameters.insert(Strings::Map::value_type(name, value));
      }
    }
  private:
    const AYLEntries::RWPtr Container;
    Strings::Map Parameters;
  };

  class ParametersFilter : public Parameters::Visitor
  {
  public:
    ParametersFilter(const VersionLayer& version, Parameters::Visitor& delegate)
      : Version(version)
      , Delegate(delegate)
      , FormatSpec()
      , Offset()
    {
    }

    std::size_t GetFormatSpec() const
    {
      return FormatSpec;
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }

    void SetValue(const Parameters::NameType& name, Parameters::IntType val) override
    {
      const String nameStr = FromStdString(name.FullPath());
      Dbg("  property %1%=%2%", nameStr, val);
      if (nameStr == AYL::CHIP_FREQUENCY)
      {
        Delegate.SetValue(Parameters::ZXTune::Core::AYM::CLOCKRATE, val);
      }
      else if (nameStr == AYL::PLAYER_FREQUENCY)
      {
        //TODO: think about this...
      }
      else if (nameStr == AYL::FORMAT_SPECIFIC)
      {
        FormatSpec = static_cast<std::size_t>(val);
      }
      else if (nameStr == AYL::OFFSET)
      {
        Offset = static_cast<std::size_t>(val);
      }
      //ignore "Loop", "Length", "Time"
    }

    void SetValue(const Parameters::NameType& name, const Parameters::StringType& val) override
    {
      const String nameStr = FromStdString(name.FullPath());
      Dbg("  property %1%='%2%'", nameStr, val);
      if (nameStr == AYL::CHIP_TYPE)
      {
        Delegate.SetValue(Parameters::ZXTune::Core::AYM::TYPE, DecodeChipType(val));
      }
      //ignore "Channels"
      else if (nameStr == AYL::CHANNELS_ALLOCATION)
      {
        Delegate.SetValue(Parameters::ZXTune::Core::AYM::LAYOUT, DecodeChipLayout(val));
      }
      //ignore "Length", "Address", "Loop", "Time", "Original"
      else if (nameStr == AYL::NAME)
      {
        Delegate.SetValue(Module::ATTR_TITLE, val);
      }
      else if (nameStr == AYL::AUTHOR)
      {
        Delegate.SetValue(Module::ATTR_AUTHOR, val);
      }
      else if (nameStr == AYL::PROGRAM || nameStr == AYL::TRACKER)
      {
        Delegate.SetValue(Module::ATTR_PROGRAM, val);
      }
      else if (nameStr == AYL::COMPUTER)
      {
        Delegate.SetValue(Module::ATTR_COMPUTER, val);
      }
      else if (nameStr == AYL::DATE)
      {
        Delegate.SetValue(Module::ATTR_DATE, val);
      }
      else if (nameStr == AYL::COMMENT)
      {
        //TODO: process escape sequence
        Delegate.SetValue(Module::ATTR_COMMENT, val);
      }
      //ignore "Tracker", "Type", "ams_andsix", "FormatSpec"
    }

    void SetValue(const Parameters::NameType& name, const Parameters::DataType& val) override
    {
      //try to process as string
      Delegate.SetValue(name, Parameters::ConvertToString(val));
    }
  private:
    static Parameters::IntType DecodeChipType(const String& value)
    {
      return value == AYL::YM ? Parameters::ZXTune::Core::AYM::TYPE_YM : Parameters::ZXTune::Core::AYM::TYPE_AY;
    }

    static Parameters::IntType DecodeChipLayout(const String& value)
    {
      if (value == AYL::ACB)
      {
        return Devices::AYM::LAYOUT_ACB;
      }
      else if (value == AYL::BAC)
      {
        return Devices::AYM::LAYOUT_BAC;
      }
      else if (value == AYL::BCA)
      {
        return Devices::AYM::LAYOUT_BCA;
      }
      else if (value == AYL::CAB)
      {
        return Devices::AYM::LAYOUT_CAB;
      }
      else if (value == AYL::CBA)
      {
        return Devices::AYM::LAYOUT_CBA;
      }
      else if (value == AYL::MONO)
      {
        return Devices::AYM::LAYOUT_MONO;
      }
      else
      {
        //default fallback
        return Devices::AYM::LAYOUT_ABC;
      }
    }
  private:
    const VersionLayer& Version;
    Parameters::Visitor& Delegate;
    std::size_t FormatSpec;
    std::size_t Offset;
  };

  Parameters::Container::Ptr CreateProperties(const VersionLayer& version, const AYLContainer& aylItems)
  {
    const Parameters::Container::Ptr properties = Parameters::Container::Create();
    ParametersFilter filter(version, *properties);
    const Strings::Map& listParams = aylItems.GetParameters();
    Parameters::Convert(listParams, filter);
    return properties;
  }

  String AppendSubpath(const String& path, const String& subpath)
  {
    try
    {
      const IO::Identifier::Ptr id = IO::ResolveUri(path);
      return id->WithSubpath(subpath)->Full();
    }
    catch (const Error&)
    {
      return path;
    }
  }

  void ApplyFormatSpecificData(std::size_t formatSpec, Playlist::IO::ContainerItem& item)
  {
    //for AY files FormatSpec is subtune index
    if (boost::algorithm::iends_with(item.Path, FromStdString(".ay")))
    {
      const auto subPath = Strings::PrefixedIndex(Text::MULTITRACK_FILENAME_PREFIX, formatSpec).ToString();
      item.Path = AppendSubpath(item.Path, subPath);
    }
  }

  void ApplyOffset(std::size_t offset, Playlist::IO::ContainerItem& item)
  {
    //offset for YM/VTX cannot be applied
    if (!boost::algorithm::iends_with(item.Path, FromStdString(".vtx")) &&
        !boost::algorithm::iends_with(item.Path, FromStdString(".ym")))
    {
      assert(offset);
      const auto subPath = Strings::PrefixedIndex(Text::RAW_PLUGIN_PREFIX, offset).ToString();
      item.Path = AppendSubpath(item.Path, subPath);
    }
  }

  Playlist::IO::ContainerItems::Ptr CreateItems(const QString& basePath, const VersionLayer& version, const AYLContainer& aylItems)
  {
    const QDir baseDir(basePath);
    const Playlist::IO::ContainerItems::RWPtr items = MakeRWPtr<Playlist::IO::ContainerItems>();
    for (AYLContainer::Iterator iter = aylItems.GetIterator(); iter.IsValid(); iter.Next())
    {
      const String& itemPath = iter.GetPath();
      Dbg("Processing '%1%'", itemPath);
      Playlist::IO::ContainerItem item;
      const Parameters::Container::Ptr adjustedParams = Parameters::Container::Create();
      ParametersFilter filter(version, *adjustedParams);
      const Strings::Map& itemParams = iter.GetParameters();
      Parameters::Convert(itemParams, filter);
      item.AdjustedParameters = adjustedParams;
      const QString absItemPath = baseDir.absoluteFilePath(ToQString(itemPath));
      item.Path = FromQString(baseDir.cleanPath(absItemPath));
      if (std::size_t formatSpec = filter.GetFormatSpec())
      {
        ApplyFormatSpecificData(formatSpec, item);
      }
      if (std::size_t offset = filter.GetOffset())
      {
        ApplyOffset(offset, item);
      }
      items->push_back(item);
    }
    return items;
  }

  bool CheckAYLByName(const QString& filename)
  {
    static const QLatin1String AYL_SUFFIX(AYL::SUFFIX);
    return filename.endsWith(AYL_SUFFIX, Qt::CaseInsensitive);
  }
}

namespace Playlist
{
  namespace IO
  {
    Container::Ptr OpenAYL(Item::DataProvider::Ptr provider, const QString& filename, Log::ProgressCallback& cb)
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
      Dbg("Processing AYL version %1%", vers);
      const VersionLayer version(vers);
      LinesSource lines(stream, version);
      const AYLContainer aylItems(lines, cb);
      const QString basePath = info.absolutePath();
      const ContainerItems::Ptr items = CreateItems(basePath, version, aylItems);
      const Parameters::Container::Ptr properties = CreateProperties(version, aylItems);
      properties->SetValue(Playlist::ATTRIBUTE_NAME, FromQString(info.baseName()));
      return Playlist::IO::CreateContainer(provider, properties, items);
    }
  }
}
