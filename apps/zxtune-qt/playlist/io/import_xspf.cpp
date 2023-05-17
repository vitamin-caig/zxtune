/**
 *
 * @file
 *
 * @brief Import .xspf implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "container_impl.h"
#include "import.h"
#include "tags/xspf.h"
#include "ui/utils.h"
// common includes
#include <error.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <module/attributes.h>
#include <parameters/convert.h>
#include <parameters/serialize.h>
#include <tools/progress_callback_helpers.h>
// std includes
#include <cctype>
#include <set>
// qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>

namespace
{
  const Debug::Stream Dbg("Playlist::IO::XSPF");

  enum
  {
    VERSION_WITH_TEXT_FIELDS_ESCAPING = 1,

    LAST_VERSION = 1
  };

  bool IsPlaylistEnabledProperty(Parameters::Identifier name)
  {
    return name == Playlist::ATTRIBUTE_VERSION || name == Playlist::ATTRIBUTE_NAME;
  };

  bool IsItemDisabledProperty(Parameters::Identifier name)
  {
    return name == Module::ATTR_CRC || name == Module::ATTR_FIXEDCRC || name == Module::ATTR_SIZE
           || name == Module::ATTR_CONTAINER || name == Module::ATTR_TYPE || name == Module::ATTR_VERSION
           || name == Module::ATTR_PROGRAM || name == Module::ATTR_DATE;
  };

  class PropertiesFilter : public Parameters::Visitor
  {
  public:
    using PropertyFilter = bool (*)(Parameters::Identifier);

    PropertiesFilter(Parameters::Visitor& delegate, PropertyFilter filter, bool match)
      : Delegate(delegate)
      , Filter(filter)
      , Match(match)
    {}

    void SetValue(Parameters::Identifier name, Parameters::IntType val) override
    {
      if (Pass(name))
      {
        Delegate.SetValue(name, val);
      }
    }

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      if (Pass(name))
      {
        Delegate.SetValue(name, val);
      }
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      if (Pass(name))
      {
        Delegate.SetValue(name, val);
      }
    }

  private:
    bool Pass(Parameters::Identifier name) const
    {
      return Match == Filter(name);
    }

  private:
    Parameters::Visitor& Delegate;
    const PropertyFilter Filter;
    const bool Match;
  };

  class XSPFReader
  {
  public:
    XSPFReader(const QFileInfo& file, QIODevice& device)
      : File(file)
      , BaseDir(File.absoluteDir())
      , XML(&device)
      , Version(LAST_VERSION)
      , Properties(Parameters::Container::Create())
      , Items(MakeRWPtr<Playlist::IO::ContainerItems>())
    {
      Properties->SetValue(Playlist::ATTRIBUTE_NAME, FromQString(File.baseName()));
    }

    bool Parse(Log::ProgressCallback& cb)
    {
      if (XML.readNextStartElement())
      {
        if (XML.name() != XSPF::ROOT_TAG)
        {
          Dbg("Invalid format");
          return false;
        }
        const QXmlStreamAttributes attributes = XML.attributes();
        if (attributes.value(QLatin1String(XSPF::VERSION_ATTR)) != XSPF::VERSION_VALUE)
        {
          Dbg("  unknown format version");
        }
        return ParsePlaylist(cb);
      }
      return !XML.error();
    }

    Playlist::IO::ContainerItems::Ptr GetItems() const
    {
      return Items;
    }

    Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

  private:
    bool ParsePlaylist(Log::ProgressCallback& cb)
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ROOT_TAG);
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::EXTENSION_TAG)
        {
          Dbg(" Parsing playlist extension");
          PropertiesFilter filter(*Properties, &IsPlaylistEnabledProperty, true);
          ParseExtension(filter);
          Properties->FindValue(Playlist::ATTRIBUTE_VERSION, Version);
        }
        else if (tagName == XSPF::TRACKLIST_TAG)
        {
          if (!ParseTracklist(cb))
          {
            Dbg("Failed to parse tracklist");
            return false;
          }
        }
        else
        {
          XML.skipCurrentElement();
        }
      }
      return !XML.error();
    }

    bool ParseTracklist(Log::ProgressCallback& cb)
    {
      assert(XML.isStartElement() && XML.name() == XSPF::TRACKLIST_TAG);
      const uint_t REPORT_PERIOD_ITEMS = 1000;
      Log::PercentProgressCallback progress(File.size(), cb);
      for (uint_t count = 0; XML.readNextStartElement(); ++count)
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::ITEM_TAG)
        {
          if (!ParseTrackItem())
          {
            Dbg("Failed to parse trackitem: {} at {}:{}", FromQString(XML.errorString()), XML.lineNumber(),
                XML.columnNumber());
            return false;
          }
        }
        else
        {
          Dbg("Unknown item in tracklist");
          XML.skipCurrentElement();
        }
        if (++count >= REPORT_PERIOD_ITEMS)
        {
          progress.OnProgress(XML.device()->pos());
          count = 0;
        }
      }
      return !XML.error();
    }

    bool ParseTrackItem()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ITEM_TAG);
      Dbg(" Parsing item");
      Playlist::IO::ContainerItem item;
      const Parameters::Container::Ptr parameters = Parameters::Container::Create();
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::ITEM_LOCATION_TAG)
        {
          item.Path = ParseTrackitemLocation();
          Dbg("  parsed location {}", item.Path);
        }
        else
        {
          ParseTrackitemParameters(tagName, *parameters);
        }
      }
      if (!item.Path.empty())
      {
        item.AdjustedParameters = parameters;
        Items->push_back(item);
      }
      return !XML.error();
    }

    String ParseTrackitemLocation()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ITEM_LOCATION_TAG);
      const QString inLocation = XML.readElementText();
      const QString location = inLocation.startsWith(XSPF::EMBEDDED_PREFIX) ? File.absoluteFilePath() + inLocation
                                                                            : inLocation;
      const QUrl url(QUrl::fromPercentEncoding(location.toUtf8()));
      const QString& itemLocation = url.isRelative() ? BaseDir.absoluteFilePath(url.toString()) : url.toString();
      return FromQString(itemLocation);
    }

    void ParseTrackitemParameters(const QStringRef& attr, Parameters::Visitor& props)
    {
      assert(XML.isStartElement() && XML.name() == attr);
      if (attr == XSPF::ITEM_CREATOR_TAG)
      {
        const String author = ConvertString(XML.readElementText());
        props.SetValue(Module::ATTR_AUTHOR, author);
        Dbg("  parsed author {}", author);
      }
      else if (attr == XSPF::ITEM_TITLE_TAG)
      {
        const String title = ConvertString(XML.readElementText());
        props.SetValue(Module::ATTR_TITLE, title);
        Dbg("  parsed title {}", title);
      }
      else if (attr == XSPF::ITEM_ANNOTATION_TAG)
      {
        const String annotation = ConvertString(XML.readElementText());
        props.SetValue(Module::ATTR_COMMENT, annotation);
        Dbg("  parsed comment {}", annotation);
      }
      else if (attr == XSPF::EXTENSION_TAG)
      {
        Dbg("  parsing extension");
        PropertiesFilter filter(props, &IsItemDisabledProperty, false);
        ParseExtension(filter);
      }
      else
      {
        if (attr != XSPF::ITEM_DURATION_TAG)
        {
          Dbg("Unknown playitem attribute '{}'", FromQString(attr.toString()));
        }
        XML.skipCurrentElement();
      }
    }

    void ParseExtension(Parameters::Visitor& props)
    {
      assert(XML.isStartElement() && XML.name() == XSPF::EXTENSION_TAG);
      if (!CheckForZXTuneExtension())
      {
        return;
      }
      Strings::Map strings;
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName != XSPF::EXTENDED_PROPERTY_TAG)
        {
          XML.skipCurrentElement();
        }
        const QXmlStreamAttributes attributes = XML.attributes();
        const QStringRef& propName = attributes.value(QLatin1String(XSPF::EXTENDED_PROPERTY_NAME_ATTR));
        const QString& propValue = XML.readElementText();
        const String propNameStr = FromQString(propName.toString());
        const String propValStr = ConvertString(propValue);
        strings[propNameStr] = propValStr;
        Dbg("  parsing extended property {}='{}'", propNameStr, propValStr);
      }
      Parameters::Convert(strings, props);
    }

    bool CheckForZXTuneExtension()
    {
      const QXmlStreamAttributes attributes = XML.attributes();
      return attributes.value(QLatin1String(XSPF::APPLICATION_ATTR)) == Playlist::APPLICATION_ID;
    }

    String ConvertString(const QString& input) const
    {
      const QString decoded = Version >= VERSION_WITH_TEXT_FIELDS_ESCAPING ? QUrl::fromPercentEncoding(input.toUtf8())
                                                                           : input;
      const String res = FromQString(decoded);
      String unescaped;
      return Parameters::ConvertFromString(res, unescaped) ? unescaped : res;
    }

  private:
    const QFileInfo File;
    const QDir BaseDir;
    QXmlStreamReader XML;
    // context
    Parameters::IntType Version;
    const Parameters::Container::Ptr Properties;
    const Playlist::IO::ContainerItems::RWPtr Items;
  };

  Playlist::IO::Container::Ptr CreateXSPFPlaylist(Playlist::Item::DataProvider::Ptr provider, const QFileInfo& fileInfo,
                                                  Log::ProgressCallback& cb)
  {
    QFile device(fileInfo.absoluteFilePath());
    if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      assert(!"Failed to open playlist");
      return Playlist::IO::Container::Ptr();
    }
    XSPFReader reader(fileInfo, device);
    if (!reader.Parse(cb))
    {
      Dbg("Failed to parse");
      return Playlist::IO::Container::Ptr();
    }

    const Playlist::IO::ContainerItems::Ptr items = reader.GetItems();
    const Parameters::Accessor::Ptr properties = reader.GetProperties();
    Dbg("Parsed {} items", items->size());
    return Playlist::IO::CreateContainer(provider, properties, items);
  }

  bool CheckXSPFByName(const QString& filename)
  {
    return filename.endsWith(XSPF::SUFFIX, Qt::CaseInsensitive);
  }
}  // namespace

namespace Playlist::IO
{
  Container::Ptr OpenXSPF(Item::DataProvider::Ptr provider, const QString& filename, Log::ProgressCallback& cb)
  {
    const QFileInfo info(filename);
    if (!info.isFile() || !info.isReadable() || !CheckXSPFByName(info.fileName()))
    {
      return Container::Ptr();
    }
    return CreateXSPFPlaylist(provider, info, cb);
  }
}  // namespace Playlist::IO
