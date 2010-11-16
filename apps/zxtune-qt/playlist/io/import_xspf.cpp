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

  const char XSPF_ROOT_TAG[] = "playlist";
  const char XSPF_VERSION_ATTR[] = "version";
  const char XSPF_VERSION_VALUE[] = "1";
  const char XSPF_TRACKLIST_TAG[] = "trackList";
  const char XSPF_ITEM_TAG[] = "track";
  const char XSPF_ITEM_LOCATION_TAG[] = "location";
  const char XSPF_ITEM_CREATOR_TAG[] = "creator";
  const char XSPF_ITEM_TITLE_TAG[] = "title";
  const char XSPF_ITEM_ANNOTATION_TAG[] = "annotation";

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
      , Items(boost::make_shared<PlaylistContainerItems>())
    {
    }

    bool Parse()
    {
      if (XML.readNextStartElement())
      {
        if (XML.name() != XSPF_ROOT_TAG ||
            XML.attributes().value(XSPF_VERSION_ATTR) != XSPF_VERSION_VALUE)
        {
          Log::Debug(THIS_MODULE, "Invalid playlist format");
          return false;
        }
        return ParsePlaylist();
      }
      return !XML.error();
    }

    PlaylistContainerItemsPtr GetItems() const
    {
      return Items;
    }

    Parameters::Accessor::Ptr GetProperties() const
    {
      return Parameters::Container::Create();
    }
  private:
    bool ParsePlaylist()
    {
      assert(XML.isStartElement() && XML.name() == XSPF_ROOT_TAG);
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF_TRACKLIST_TAG &&
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
      assert(XML.isStartElement() && XML.name() == XSPF_TRACKLIST_TAG);
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF_ITEM_TAG &&
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
      assert(XML.isStartElement() && XML.name() == XSPF_ITEM_TAG);
      PlaylistContainerItem item;
      const Parameters::Container::Ptr attributes = Parameters::Container::Create();
      while (XML.readNextStartElement())
      {
        const QStringRef& tagName = XML.name();
        if (tagName == XSPF_ITEM_LOCATION_TAG)
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
      assert(XML.isStartElement() && XML.name() == XSPF_ITEM_LOCATION_TAG);
      const QString& location = XML.readElementText();
      return FromQString(ConcatenatePath(BasePath, location));
    }

    void ParseTrackitemAttribute(const QStringRef& attr, Parameters::Modifier& props)
    {
      assert(XML.isStartElement() && XML.name() == attr);
      if (attr == XSPF_ITEM_CREATOR_TAG)
      {
        const String author = FromQString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_AUTHOR, author);
      }
      else if (attr == XSPF_ITEM_TITLE_TAG)
      {
        const String title = FromQString(XML.readElementText());
        props.SetStringValue(ZXTune::Module::ATTR_TITLE, title);
      }
      else if (attr == XSPF_ITEM_ANNOTATION_TAG)
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
    const boost::shared_ptr<PlaylistContainerItems> Items;
  };

  PlaylistIOContainer::Ptr CreateXSPFPlaylist(PlayitemsProvider::Ptr provider,
    const QString& basePath, QIODevice& device)
  {
    XSPFReader reader(basePath, device);
    if (!reader.Parse())
    {
      return PlaylistIOContainer::Ptr();
    }
    return CreatePlaylistIOContainer(provider, reader.GetProperties(), reader.GetItems());
  }

  bool CheckXSPFByName(const QString& filename)
  {
    static const QString XSPF_SUFFIX = QString::fromUtf8(".xspf");
    return filename.endsWith(XSPF_SUFFIX, Qt::CaseInsensitive);
  }
}

PlaylistIOContainer::Ptr OpenXSPFPlaylist(PlayitemsProvider::Ptr provider, const QString& filename)
{
  const QFileInfo info(filename);
  if (!info.isFile() || !info.isReadable() ||
      !CheckXSPFByName(info.fileName()))
  {
    return PlaylistIOContainer::Ptr();
  }
  QFile device(filename);
  if (!device.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    assert(!"Failed to open playlist");
    return PlaylistIOContainer::Ptr();
  }
  const QString basePath = info.absolutePath();
  return CreateXSPFPlaylist(provider, basePath, device);
}
