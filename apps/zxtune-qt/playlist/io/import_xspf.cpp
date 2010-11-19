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
#include <QtCore/QXmlStreamReader>

namespace
{
  const std::string THIS_MODULE("Playlist::IO::XSPF");

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
      , Items(boost::make_shared<Playlist::IO::ContainerItems>())
    {
    }

    bool Parse()
    {
      if (XML.readNextStartElement())
      {
        if (XML.name() != XSPF::ROOT_TAG)
        {
          Log::Debug(THIS_MODULE, "Invalid playlist format");
          return false;
        }
        if (XML.attributes().value(XSPF::VERSION_ATTR) != XSPF::VERSION_VALUE)
        {
          Log::Debug(THIS_MODULE, "  unknown version");
        }
        return ParsePlaylist();
      }
      return !XML.error();
    }

    Playlist::IO::ContainerItemsPtr GetItems() const
    {
      return Items;
    }
  private:
    bool ParsePlaylist()
    {
      assert(XML.isStartElement() && XML.name() == XSPF::ROOT_TAG);
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF::TRACKLIST_TAG &&
            !ParseTracklist())
        {
          Log::Debug(THIS_MODULE, "Failed to parse tracklist");
          return false;
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
        if (tagName == XSPF::ITEM_TAG &&
            !ParseTrackItem())
        {
          Log::Debug(THIS_MODULE, "Failed to parse trackitem");
          return false;
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
      const QString& location = XML.readElementText();
      return FromQString(ConcatenatePath(BasePath, location));
    }

    void ParseTrackitemAttribute(const QStringRef& attr, Parameters::Modifier& props)
    {
      assert(XML.isStartElement() && XML.name() == attr);
      if (attr == XSPF::ITEM_CREATOR_TAG)
      {
        const String author = FromQString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_AUTHOR, author);
      }
      else if (attr == XSPF::ITEM_TITLE_TAG)
      {
        const String title = FromQString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_TITLE, title);
      }
      else if (attr == XSPF::ITEM_ANNOTATION_TAG)
      {
        const String annotation = FromQString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_COMMENT, annotation);
      }
      else
      {
        Log::Debug(THIS_MODULE, "Unknown playitem attribute '%1%'", FromQString(attr.toString()));
      }
    }
  private:
    const QString BasePath;
    QXmlStreamReader XML;
    //context
    const boost::shared_ptr<Playlist::IO::ContainerItems> Items;
  };

  Playlist::IO::Container::Ptr CreateXSPFPlaylist(PlayitemsProvider::Ptr provider,
    const QFileInfo& fileInfo)
  {
    const QString basePath = fileInfo.absolutePath();
    QFile device(fileInfo.absoluteFilePath());
    if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      assert(!"Failed to open XSPF playlist");
      return Playlist::IO::Container::Ptr();
    }
    XSPFReader reader(basePath, device);
    if (!reader.Parse())
    {
      return Playlist::IO::Container::Ptr();
    }

    const Parameters::Container::Ptr properties = Parameters::Container::Create();
    const Playlist::IO::ContainerItemsPtr items = reader.GetItems();
    properties->SetStringValue(Playlist::ATTRIBUTE_NAME, FromQString(fileInfo.baseName()));
    properties->SetIntValue(Playlist::ATTRIBUTE_SIZE, items->size());
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
    Container::Ptr OpenXSPF(PlayitemsProvider::Ptr provider, const QString& filename)
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
