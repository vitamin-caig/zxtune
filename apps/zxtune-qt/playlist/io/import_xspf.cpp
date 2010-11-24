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

  QString ConcatenatePath(const QString& baseDirPath, const QString& subPath)
  {
    const QDir baseDir(baseDirPath);
    const QFileInfo file(baseDir, subPath);
    return file.canonicalFilePath();
  }

  class XSPFReader
  {
  public:
    XSPFReader(const QString& basePath, QIODevice& device)
      : BasePath(basePath)
      , XML(&device)
      , Version(0)
      , Properties(Parameters::Container::Create())
      , Items(boost::make_shared<Playlist::IO::ContainerItems>())
    {
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
        if (XML.attributes().value(XSPF::VERSION_ATTR) != XSPF::VERSION_VALUE)
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
          ParseExtension(*Properties);
          Properties->FindIntValue(Playlist::ATTRIBUTE_VERSION, Version);
          Log::Debug(THIS_MODULE, "Version %1%", Version);
        }
        else if (tagName == XSPF::TRACKLIST_TAG)
        {
          if (!ParseTracklist())
          {
            Log::Debug(THIS_MODULE, "Failed to parse tracklist");
            return false;
          }
          Properties->SetIntValue(Playlist::ATTRIBUTE_SIZE, Items->size());
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
      Playlist::IO::ContainerItem item;
      const Parameters::Container::Ptr attributes = Parameters::Container::Create();
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::ITEM_LOCATION_TAG)
        {
          item.Path = ParseTrackitemLocation();
        }
        else
        {
          ParseTrackitemAttribute(tagName, *attributes);
        }
      }
      if (!item.Path.empty())
      {
        item.AdjustedParameters = attributes;
        Items->push_back(item);
      }
      return !XML.error();
    }

    String ParseTrackitemLocation()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ITEM_LOCATION_TAG);
      const QUrl url(XML.readElementText());
      return FromQString(ConcatenatePath(BasePath, url.toString()));
    }

    void ParseTrackitemAttribute(const QStringRef& attr, Parameters::Modifier& props)
    {
      assert(XML.isStartElement() && XML.name() == attr);
      if (attr == XSPF::ITEM_CREATOR_TAG)
      {
        const String author = ConvertString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_AUTHOR, author);
      }
      else if (attr == XSPF::ITEM_TITLE_TAG)
      {
        const String title = ConvertString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_TITLE, title);
      }
      else if (attr == XSPF::ITEM_ANNOTATION_TAG)
      {
        const String annotation = ConvertString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_COMMENT, annotation);
      }
      else if (attr == XSPF::EXTENSION_TAG)
      {
        ParseExtension(props);
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

    void ParseExtension(Parameters::Modifier& props)
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
        const QStringRef& propName = XML.attributes().value(XSPF::EXTENDED_PROPERTY_NAME_ATTR);
        const QString& propValue = XML.readElementText();
        strings[FromQString(propName.toString())] = ConvertString(propValue);
      }
      Parameters::ParseStringMap(strings, props);
    }

    bool CheckForZXTuneExtension()
    {
      return XML.attributes().value(XSPF::APPLICATION_ATTR) == Text::PROGRAM_SITE;
    }

    String ConvertString(const QString& input) const
    {
      const QString decoded = Version >= VERSION_WITH_TEXT_FIELDS_ESCAPING
        ? QUrl::fromPercentEncoding(input.toUtf8())
        : input;
      return FromQString(decoded);
    }
  private:
    const QString BasePath;
    QXmlStreamReader XML;
    //context
    Parameters::IntType Version;
    const Parameters::Container::Ptr Properties;
    const boost::shared_ptr<Playlist::IO::ContainerItems> Items;
  };

  Playlist::IO::Container::Ptr CreateXSPFPlaylist(Playlist::Item::DataProvider::Ptr provider,
    const QFileInfo& fileInfo)
  {
    const QString basePath = fileInfo.absolutePath();
    QFile device(fileInfo.absoluteFilePath());
    if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      assert(!"Failed to open playlist");
      return Playlist::IO::Container::Ptr();
    }
    XSPFReader reader(basePath, device);
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
