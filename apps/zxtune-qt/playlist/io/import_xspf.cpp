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
#include <logging.h>
//library includes
#include <core/module_attrs.h>
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
  const std::string THIS_MODULE("Playlist::IO::XSPF");

  enum
  {
    VERSION_WITH_TEXT_FIELDS_ESCAPING = 1
  };

  const Char* PLAYLIST_ENABLED_PROPERTIES[] =
  {
    Playlist::ATTRIBUTE_VERSION,
    Playlist::ATTRIBUTE_NAME,
  };

  const Char* ITEM_DISABLED_PROPERTIES[] =
  {
    ZXTune::Module::ATTR_CRC,
    ZXTune::Module::ATTR_FIXEDCRC,
    ZXTune::Module::ATTR_SIZE,
    ZXTune::Module::ATTR_CONTAINER,
    ZXTune::Module::ATTR_TYPE,
    ZXTune::Module::ATTR_VERSION,
    ZXTune::Module::ATTR_PROGRAM,
    ZXTune::Module::ATTR_DATE
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
    bool Pass(const String& name) const
    {
      return Match == (0 != Filter.count(name));
    }
  private:
    Parameters::Visitor& Delegate;
    const std::set<String> Filter;
    const bool Match;
  };

  class XSPFReader
  {
  public:
    XSPFReader(const QString& basePath, const QString& autoName, QIODevice& device)
      : BaseDir(basePath)
      , XML(&device)
      , Version(0)
      , Properties(Parameters::Container::Create())
      , Items(boost::make_shared<Playlist::IO::ContainerItems>())
    {
      Properties->SetValue(Playlist::ATTRIBUTE_NAME, FromQString(autoName));
    }

    bool Parse()
    {
      if (XML.readNextStartElement())
      {
        if (XML.name() != XSPF::ROOT_TAG)
        {
          Log::Debug(THIS_MODULE, "Invalid format");
          return false;
        }
        const QXmlStreamAttributes attributes = XML.attributes();
        if (attributes.value(XSPF::VERSION_ATTR) != XSPF::VERSION_VALUE)
        {
          Log::Debug(THIS_MODULE, "  unknown format version");
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
          Log::Debug(THIS_MODULE, " Parsing playlist extension");
          PropertiesFilter filter(*Properties, PLAYLIST_ENABLED_PROPERTIES, ArrayEnd(PLAYLIST_ENABLED_PROPERTIES), true);
          ParseExtension(filter);
          Properties->FindValue(Playlist::ATTRIBUTE_VERSION, Version);
        }
        else if (tagName == XSPF::TRACKLIST_TAG)
        {
          if (!ParseTracklist())
          {
            Log::Debug(THIS_MODULE, "Failed to parse tracklist");
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
            Log::Debug(THIS_MODULE, "Failed to parse trackitem: %1% at %2%:%3%",
              FromQString(XML.errorString()), XML.lineNumber(), XML.columnNumber());
            return false;
          }
        }
        else
        {
          Log::Debug(THIS_MODULE, "Unknown item in tracklist");
          XML.skipCurrentElement();
        }
      }
      return !XML.error();
    }

    bool ParseTrackItem()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ITEM_TAG);
      Log::Debug(THIS_MODULE, " Parsing item");
      Playlist::IO::ContainerItem item;
      const Parameters::Container::Ptr parameters = Parameters::Container::Create();
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::ITEM_LOCATION_TAG)
        {
          item.Path = ParseTrackitemLocation();
          Log::Debug(THIS_MODULE, "  parsed location %1%", item.Path);
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
      const QString location = XML.readElementText();;
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
        props.SetValue(ZXTune::Module::ATTR_AUTHOR, author);
        Log::Debug(THIS_MODULE, "  parsed author %1%", author);
     }
      else if (attr == XSPF::ITEM_TITLE_TAG)
      {
        const String title = ConvertString(XML.readElementText());
        props.SetValue(ZXTune::Module::ATTR_TITLE, title);
        Log::Debug(THIS_MODULE, "  parsed title %1%", title);
      }
      else if (attr == XSPF::ITEM_ANNOTATION_TAG)
      {
        const String annotation = ConvertString(XML.readElementText());
        props.SetValue(ZXTune::Module::ATTR_COMMENT, annotation);
        Log::Debug(THIS_MODULE, "  parsed comment %1%", annotation);
      }
      else if (attr == XSPF::EXTENSION_TAG)
      {
        Log::Debug(THIS_MODULE, "  parsing extension");
        PropertiesFilter filter(props, ITEM_DISABLED_PROPERTIES, ArrayEnd(ITEM_DISABLED_PROPERTIES), false);
        ParseExtension(filter);
      }
      else
      {
        if (attr != XSPF::ITEM_DURATION_TAG)
        {
          Log::Debug(THIS_MODULE, "Unknown playitem attribute '%1%'", FromQString(attr.toString()));
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
      StringMap strings;
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName != XSPF::EXTENDED_PROPERTY_TAG)
        {
          XML.skipCurrentElement();
        }
        const QXmlStreamAttributes attributes = XML.attributes();
        const QStringRef& propName = attributes.value(XSPF::EXTENDED_PROPERTY_NAME_ATTR);
        const QString& propValue = XML.readElementText();
        const String propNameStr = FromQString(propName.toString());
        const String propValStr = ConvertString(propValue);
        strings[propNameStr] = propValStr;
        Log::Debug(THIS_MODULE, "  parsing extended property %1%='%2%'",
          propNameStr, propValStr);
      }
      Parameters::ParseStringMap(strings, props);
    }

    bool CheckForZXTuneExtension()
    {
      const QXmlStreamAttributes attributes = XML.attributes();
      return attributes.value(XSPF::APPLICATION_ATTR) == Text::PROGRAM_SITE;
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
    const QString basePath = fileInfo.absolutePath();
    const QString autoName = fileInfo.baseName();
    XSPFReader reader(basePath, autoName, device);
    if (!reader.Parse())
    {
      Log::Debug(THIS_MODULE, "Failed to parse");
      return Playlist::IO::Container::Ptr();
    }

    const Playlist::IO::ContainerItemsPtr items = reader.GetItems();
    const Parameters::Accessor::Ptr properties = reader.GetProperties();
    Log::Debug(THIS_MODULE, "Parsed %1% items", items->size());
    return Playlist::IO::CreateContainer(provider, properties, items);
  }

  bool CheckXSPFByName(const QString& filename)
  {
    static const QString XSPF_SUFFIX = QString::fromUtf8(XSPF::SUFFIX);
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
