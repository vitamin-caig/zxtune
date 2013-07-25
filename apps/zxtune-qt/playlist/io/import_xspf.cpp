/*
Abstract:
  Playlist import for .xspf format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "import.h"
#include "container_impl.h"
#include "tags/xspf.h"
#include "ui/utils.h"
//common includes
#include <error.h>
//library includes
#include <core/module_attrs.h>
#include <debug/log.h>
//std includes
#include <cctype>
#include <set>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>
//text includes
#include "text/text.h"

namespace
{
  const Debug::Stream Dbg("Playlist::IO::XSPF");

  enum
  {
    VERSION_WITH_TEXT_FIELDS_ESCAPING = 1,

    LAST_VERSION = 1
  };

  Parameters::NameType PLAYLIST_ENABLED_PROPERTIES[] =
  {
    Playlist::ATTRIBUTE_VERSION,
    Playlist::ATTRIBUTE_NAME,
  };

  std::string ITEM_DISABLED_PROPERTIES[] =
  {
    Module::ATTR_CRC,
    Module::ATTR_FIXEDCRC,
    Module::ATTR_SIZE,
    Module::ATTR_CONTAINER,
    Module::ATTR_TYPE,
    Module::ATTR_VERSION,
    Module::ATTR_PROGRAM,
    Module::ATTR_DATE
  };

  class PropertiesFilter : public Parameters::Visitor
  {
  public:
    template<class T>
    PropertiesFilter(Parameters::Visitor& delegate, T propFrom, T propTo, bool match)
      : Delegate(delegate)
      , Filter(propFrom, propTo)
      , Match(match)
    {
    }

    virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      if (Pass(name))
      {
        Delegate.SetValue(name, val);
      }
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      if (Pass(name))
      {
        Delegate.SetValue(name, val);
      }
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      if (Pass(name))
      {
        Delegate.SetValue(name, val);
      }
    }
  private:
    bool Pass(const Parameters::NameType& name) const
    {
      return Match == (0 != Filter.count(name));
    }
  private:
    Parameters::Visitor& Delegate;
    const std::set<Parameters::NameType> Filter;
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
      , Items(boost::make_shared<Playlist::IO::ContainerItems>())
    {
      Properties->SetValue(Playlist::ATTRIBUTE_NAME, FromQString(File.baseName()));
    }

    bool Parse()
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
        return ParsePlaylist();
      }
      return !XML.error();
    }

    Playlist::IO::ContainerItemsPtr GetItems() const
    {
      return Items;
    }

    Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }
  private:
    bool ParsePlaylist()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ROOT_TAG);
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::EXTENSION_TAG)
        {
          Dbg(" Parsing playlist extension");
          PropertiesFilter filter(*Properties, PLAYLIST_ENABLED_PROPERTIES, ArrayEnd(PLAYLIST_ENABLED_PROPERTIES), true);
          ParseExtension(filter);
          Properties->FindValue(Playlist::ATTRIBUTE_VERSION, Version);
        }
        else if (tagName == XSPF::TRACKLIST_TAG)
        {
          if (!ParseTracklist())
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

    bool ParseTracklist()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::TRACKLIST_TAG);
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::ITEM_TAG)
        {
          if (!ParseTrackItem())
          {
            Dbg("Failed to parse trackitem: %1% at %2%:%3%",
              FromQString(XML.errorString()), XML.lineNumber(), XML.columnNumber());
            return false;
          }
        }
        else
        {
          Dbg("Unknown item in tracklist");
          XML.skipCurrentElement();
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
          Dbg("  parsed location %1%", item.Path);
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
      const QString location = inLocation.startsWith(XSPF::EMBEDDED_PREFIX) ? File.absoluteFilePath() + inLocation : inLocation;
      const QUrl url(QUrl::fromPercentEncoding(location.toUtf8()));
      const QString& itemLocation = url.isRelative()
        ? BaseDir.absoluteFilePath(url.toString())
        : url.toString();
      return FromQString(itemLocation);
    }

    void ParseTrackitemParameters(const QStringRef& attr, Parameters::Visitor& props)
    {
      assert(XML.isStartElement() && XML.name() == attr);
      if (attr == XSPF::ITEM_CREATOR_TAG)
      {
        const String author = ConvertString(XML.readElementText());
        props.SetValue(Module::ATTR_AUTHOR, author);
        Dbg("  parsed author %1%", author);
     }
      else if (attr == XSPF::ITEM_TITLE_TAG)
      {
        const String title = ConvertString(XML.readElementText());
        props.SetValue(Module::ATTR_TITLE, title);
        Dbg("  parsed title %1%", title);
      }
      else if (attr == XSPF::ITEM_ANNOTATION_TAG)
      {
        const String annotation = ConvertString(XML.readElementText());
        props.SetValue(Module::ATTR_COMMENT, annotation);
        Dbg("  parsed comment %1%", annotation);
      }
      else if (attr == XSPF::EXTENSION_TAG)
      {
        Dbg("  parsing extension");
        PropertiesFilter filter(props, ITEM_DISABLED_PROPERTIES, ArrayEnd(ITEM_DISABLED_PROPERTIES), false);
        ParseExtension(filter);
      }
      else
      {
        if (attr != XSPF::ITEM_DURATION_TAG)
        {
          Dbg("Unknown playitem attribute '%1%'", FromQString(attr.toString()));
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
        Dbg("  parsing extended property %1%='%2%'",
          propNameStr, propValStr);
      }
      Parameters::ParseStringMap(strings, props);
    }

    bool CheckForZXTuneExtension()
    {
      const QXmlStreamAttributes attributes = XML.attributes();
      return attributes.value(QLatin1String(XSPF::APPLICATION_ATTR)) == Text::PROGRAM_SITE;
    }

    String ConvertString(const QString& input) const
    {
      const QString decoded = Version >= VERSION_WITH_TEXT_FIELDS_ESCAPING
        ? QUrl::fromPercentEncoding(input.toUtf8())
        : input;
      const String res = FromQString(decoded);
      String unescaped;
      return Parameters::ConvertFromString(res, unescaped)
        ? unescaped
        : res;
    }
  private:
    const QFileInfo File;
    const QDir BaseDir;
    QXmlStreamReader XML;
    //context
    Parameters::IntType Version;
    const Parameters::Container::Ptr Properties;
    const boost::shared_ptr<Playlist::IO::ContainerItems> Items;
  };

  Playlist::IO::Container::Ptr CreateXSPFPlaylist(Playlist::Item::DataProvider::Ptr provider,
    const QFileInfo& fileInfo)
  {
    QFile device(fileInfo.absoluteFilePath());
    if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      assert(!"Failed to open playlist");
      return Playlist::IO::Container::Ptr();
    }
    XSPFReader reader(fileInfo, device);
    if (!reader.Parse())
    {
      Dbg("Failed to parse");
      return Playlist::IO::Container::Ptr();
    }

    const Playlist::IO::ContainerItemsPtr items = reader.GetItems();
    const Parameters::Accessor::Ptr properties = reader.GetProperties();
    Dbg("Parsed %1% items", items->size());
    return Playlist::IO::CreateContainer(provider, properties, items);
  }

  bool CheckXSPFByName(const QString& filename)
  {
    static const QLatin1String XSPF_SUFFIX(XSPF::SUFFIX);
    return filename.endsWith(XSPF_SUFFIX, Qt::CaseInsensitive);
  }
}

namespace Playlist
{
  namespace IO
  {
    Container::Ptr OpenXSPF(Item::DataProvider::Ptr provider, const QString& filename)
    {
      const QFileInfo info(filename);
      if (!info.isFile() || !info.isReadable() ||
          !CheckXSPFByName(info.fileName()))
      {
        return Container::Ptr();
      }
      return CreateXSPFPlaylist(provider, info);
    }
  }
}
