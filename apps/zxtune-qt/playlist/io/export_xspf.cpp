/**
* 
* @file
*
* @brief Implementation of .xspf exporting
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "export.h"
#include "tags/xspf.h"
#include "ui/utils.h"
//common includes
#include <error_tools.h>
//library includes
#include <zxtune.h>
#include <core/module_attrs.h>
#include <debug/log.h>
#include <sound/sound_parameters.h>
#include <parameters/convert.h>
#include <core/plugins/containers/zdata_supp.h>
//std includes
#include <set>
//boost includes
#include <boost/scoped_ptr.hpp>
//qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QXmlStreamWriter>
//text includes
#include "text/text.h"

#define FILE_TAG A016CAAF

namespace
{
  const Debug::Stream Dbg("Playlist::IO::XSPF");

  const QLatin1String ENDL("\n");

  const unsigned XSPF_VERSION = 1;

  typedef bool (*AttributesFilter)(const Parameters::NameType&);

  QString DataToQString(const QByteArray& data)
  {
    return QString::fromAscii(data.data(), data.size());
  }

  QString ConvertString(const String& str)
  {
    const QByteArray data = QUrl::toPercentEncoding(ToQString(str));
    return DataToQString(data);
  }

  class ExtendedPropertiesSaver : public Parameters::Visitor
  {
    class StringPropertySaver
    {
    public:
      explicit StringPropertySaver(QXmlStreamWriter& xml)
        : XML(xml)
      {
        XML.writeStartElement(QLatin1String(XSPF::EXTENSION_TAG));
        XML.writeAttribute(QLatin1String(XSPF::APPLICATION_ATTR), QLatin1String(Text::PLAYLIST_APPLICATION_ID));
      }

      ~StringPropertySaver()
      {
        XML.writeEndElement();
      }

      void SaveProperty(const Parameters::NameType& name, const String& strVal)
      {
        XML.writeStartElement(QLatin1String(XSPF::EXTENDED_PROPERTY_TAG));
        XML.writeAttribute(QLatin1String(XSPF::EXTENDED_PROPERTY_NAME_ATTR), ToQString(name.FullPath()));
        XML.writeCharacters(ConvertString(strVal));
        XML.writeEndElement();
      }
    private:
      QXmlStreamWriter& XML;
    };
  public:
    ExtendedPropertiesSaver(QXmlStreamWriter& xml, AttributesFilter filter = 0)
      : XML(xml)
      , Filter(filter)
    {
    }

    virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Dbg("  saving extended attribute %1%=%2%", name.FullPath(), val);
      SaveProperty(name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Dbg("  saving extended attribute %1%='%2%'", name.FullPath(), val);
      SaveProperty(name, val);
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Dbg("  saving extended attribute %1%=data(%2%)", name.FullPath(), val.size());
      SaveProperty(name, val);
    }
  private:
    template<class T>
    void SaveProperty(const Parameters::NameType& name, const T& value)
    {
      if (!Saver)
      {
        Saver.reset(new StringPropertySaver(XML));
      }
      const String strVal = Parameters::ConvertToString(value);
      Saver->SaveProperty(name, strVal);
    }
  private:
    QXmlStreamWriter& XML;
    const AttributesFilter Filter;
    boost::scoped_ptr<StringPropertySaver> Saver;
  };

  class ItemPropertiesSaver : private Parameters::Visitor
  {
  public:
    explicit ItemPropertiesSaver(QXmlStreamWriter& xml)
      : XML(xml)
    {
      XML.writeStartElement(QLatin1String(XSPF::ITEM_TAG));
    }

    virtual ~ItemPropertiesSaver()
    {
      XML.writeEndElement();
    }

    void SaveModuleLocation(const String& location)
    {
      Dbg("  saving absolute item location %1%", location);
      SaveModuleLocation(ToQString(location));
    }

    void SaveModuleLocation(const String& location, const QDir& root)
    {
      Dbg("  saving relative item location %1%", location);
      const QString path = ToQString(location);
      const QString rel = root.relativeFilePath(path);
      if (path == root.absoluteFilePath(rel))
      {
        SaveModuleLocation(rel);
      }
      else
      {
        SaveModuleLocation(path);
      }
    }

    void SaveModuleProperties(const Module::Information& info, const Parameters::Accessor& props)
    {
      //save common properties
      Dbg(" Save basic properties");
      props.Process(*this);
      SaveDuration(info, props);
      Dbg(" Save extended properties");
      SaveExtendedProperties(props);
    }

    void SaveStubModuleProperties(const Parameters::Accessor& props)
    {
      Dbg(" Save basic stub properties");
      props.Process(*this);
      Dbg(" Save stub extended properties");
      SaveExtendedProperties(props);
    }

    void SaveAdjustedParameters(const Parameters::Accessor& params)
    {
      Dbg(" Save adjusted parameters");
      ExtendedPropertiesSaver saver(XML, &KeepOnlyParameters);
      params.Process(saver);
    }

    void SaveData(const Binary::Data& content)
    {
      Dbg(" Save content");
      XML.writeCharacters(ENDL);
      XML.writeCDATA(QString::fromAscii(static_cast<const char*>(content.Start()), content.Size()));
      XML.writeCharacters(ENDL);
    }
  private:
    void SaveModuleLocation(const QString& location)
    {
      XML.writeTextElement(QLatin1String(XSPF::ITEM_LOCATION_TAG), DataToQString(QUrl(location).toEncoded()));
    }

    virtual void SetValue(const Parameters::NameType& /*name*/, Parameters::IntType /*val*/)
    {
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      const String value = Parameters::ConvertToString(val);
      const QString valStr = ConvertString(value);
      if (name == Module::ATTR_TITLE)
      {
        Dbg("  saving item attribute %1%='%2%'", name.FullPath(), val);
        SaveText(XSPF::ITEM_TITLE_TAG, valStr);
      }
      else if (name == Module::ATTR_AUTHOR)
      {
        Dbg("  saving item attribute %1%='%2%'", name.FullPath(), val);
        SaveText(XSPF::ITEM_CREATOR_TAG, valStr);
      }
      else if (name == Module::ATTR_COMMENT)
      {
        Dbg("  saving item attribute %1%='%2%'", name.FullPath(), val);
        SaveText(XSPF::ITEM_ANNOTATION_TAG, valStr);
      }
    }

    virtual void SetValue(const Parameters::NameType& /*name*/, const Parameters::DataType& /*val*/)
    {
    }
  private:
    void SaveDuration(const Module::Information& info, const Parameters::Accessor& props)
    {
      Parameters::IntType frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
      props.FindValue(Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration);
      const uint64_t msecDuration = info.FramesCount() * frameDuration / 1000;
      Dbg("  saving item attribute Duration=%1%", msecDuration);
      XML.writeTextElement(QLatin1String(XSPF::ITEM_DURATION_TAG), QString::number(msecDuration));
    }

    void SaveText(const Char* tag, const QString& value)
    {
      XML.writeTextElement(QLatin1String(tag), value);
    }

    static bool KeepExtendedProperties(const Parameters::NameType& name)
    {
      return
        //skip path-related properties
        name != Module::ATTR_FULLPATH &&
        name != Module::ATTR_PATH &&
        name != Module::ATTR_FILENAME &&
        name != Module::ATTR_EXTENSION &&
        name != Module::ATTR_SUBPATH &&
        //skip existing properties
        name != Module::ATTR_AUTHOR &&
        name != Module::ATTR_TITLE &&
        name != Module::ATTR_COMMENT &&
        //skip redundand properties
        name != Module::ATTR_CONTENT &&
        //skip all the parameters
        !IsParameter(name)
      ;
    }

    static bool IsParameter(const Parameters::NameType& name)
    {
      return name.IsPath();
    }

    static bool KeepOnlyParameters(const Parameters::NameType& name)
    {
      return name.IsSubpathOf(Parameters::ZXTune::PREFIX) &&
            !name.IsSubpathOf(Playlist::ATTRIBUTES_PREFIX);
    }

    void SaveExtendedProperties(const Parameters::Accessor& props)
    {
      ExtendedPropertiesSaver saver(XML, &KeepExtendedProperties);
      props.Process(saver);
    }
  private:
    QXmlStreamWriter& XML;
  };

  class ItemWriter
  {
  public:
    virtual ~ItemWriter() {}

    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const = 0;
  };

  class ItemFullLocationWriter : public ItemWriter
  {
  public:
    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const
    {
      saver.SaveModuleLocation(item.GetFullPath());
    }
  };

  class ItemRelativeLocationWriter : public ItemWriter
  {
  public:
    explicit ItemRelativeLocationWriter(const QString& dirName)
      : Root(dirName)
    {
    }

    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const
    {
      saver.SaveModuleLocation(item.GetFullPath(), Root);
    }
  private:
    const QDir Root;
  };

  class ItemContentLocationWriter : public ItemWriter
  {
  public:
    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const
    {
      if (const Module::Holder::Ptr holder = item.GetModule())
      {
        const Binary::Data::Ptr rawContent = Module::GetRawData(*holder);
        const ZXTune::DataLocation::Ptr container = ZXTune::BuildZdataContainer(*rawContent);
        const String id = container->GetPath()->AsString();
        saver.SaveModuleLocation(XSPF::EMBEDDED_PREFIX + id);
        if (Ids.insert(id).second)
        {
          saver.SaveData(*container->GetData());
        }
        else
        {
          Dbg("Use already stored data for id=%1%", id);
        }
      }
      else
      {
        static const ItemFullLocationWriter fallback;
        fallback.Save(item, saver);
      }
    }
  private:
    mutable std::set<String> Ids;
  };

  class ItemShortPropertiesWriter : public ItemWriter
  {
  public:
    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const
    {
      const Parameters::Accessor::Ptr adjustedParams = item.GetAdjustedParameters();
      saver.SaveStubModuleProperties(*adjustedParams);
      saver.SaveAdjustedParameters(*adjustedParams);
    }
  };

  class ItemFullPropertiesWriter : public ItemWriter
  {
  public:
    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const
    {
      if (const Module::Holder::Ptr holder = item.GetModule())
      {
        const Module::Information::Ptr info = holder->GetModuleInformation();
        const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
        saver.SaveModuleProperties(*info, *props);
        const Parameters::Accessor::Ptr adjustedParams = item.GetAdjustedParameters();
        saver.SaveAdjustedParameters(*adjustedParams);
      }
      else
      {
        static const ItemShortPropertiesWriter fallback;
        fallback.Save(item, saver);
      }
    }
  };

  class ItemCompositeWriter : public ItemWriter
  {
  public:
    ItemCompositeWriter(std::auto_ptr<ItemWriter> loc, std::auto_ptr<ItemWriter> props)
      : Location(loc)
      , Properties(props)
    {
    }

    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const
    {
      Location->Save(item, saver);
      Properties->Save(item, saver);
    }
  private:
    const std::auto_ptr<ItemWriter> Location;
    const std::auto_ptr<ItemWriter> Properties;
  };

  std::auto_ptr<const ItemWriter> CreateWriter(const QString& filename, Playlist::IO::ExportFlags flags)
  {
    std::auto_ptr<ItemWriter> location;
    std::auto_ptr<ItemWriter> props;
    if (0 != (flags & Playlist::IO::SAVE_CONTENT))
    {
      location.reset(new ItemContentLocationWriter());
      props.reset(new ItemShortPropertiesWriter());
    }
    else
    {
      if (0 != (flags & Playlist::IO::SAVE_ATTRIBUTES))
      {
        props.reset(new ItemFullPropertiesWriter());
      }
      else
      {
        props.reset(new ItemShortPropertiesWriter());
      }
      if (0 != (flags & Playlist::IO::RELATIVE_PATHS))
      {
        location.reset(new ItemRelativeLocationWriter(QFileInfo(filename).absolutePath()));
      }
      else
      {
        location.reset(new ItemFullLocationWriter());
      }
    }
    return std::auto_ptr<const ItemWriter>(new ItemCompositeWriter(location, props));
  }

  class XSPFWriter
  {
  public:
    XSPFWriter(QIODevice& device, const ItemWriter& writer)
      : XML(&device)
      , Writer(writer)
    {
      XML.setAutoFormatting(true);
      XML.setAutoFormattingIndent(2);
      XML.writeStartDocument();
      XML.writeStartElement(QLatin1String(XSPF::ROOT_TAG));
      XML.writeAttribute(QLatin1String(XSPF::VERSION_ATTR), QLatin1String(XSPF::VERSION_VALUE));
      XML.writeAttribute(QLatin1String(XSPF::XMLNS_ATTR), QLatin1String(XSPF::XMLNS_VALUE));
    }

    void WriteProperties(const Parameters::Accessor& props, uint_t items)
    {
      ExtendedPropertiesSaver saver(XML);
      props.Process(saver);
      saver.SetValue(Playlist::ATTRIBUTE_VERSION, XSPF_VERSION);
      saver.SetValue(Playlist::ATTRIBUTE_ITEMS, items);
    }

    void WriteItems(const Playlist::IO::Container& container, Playlist::IO::ExportCallback& cb)
    {
      XML.writeStartElement(QLatin1String(XSPF::TRACKLIST_TAG));
      unsigned doneItems = 0;
      for (Playlist::Item::Collection::Ptr items = container.GetItems(); items->IsValid(); items->Next())
      {
        const Playlist::Item::Data::Ptr item = items->Get();
        WriteItem(item);
        cb.Progress(++doneItems);
      }
      XML.writeEndElement();
    }

    ~XSPFWriter()
    {
      XML.writeEndDocument();
    }
  private:
    void WriteItem(Playlist::Item::Data::Ptr item)
    {
      Dbg("Save playitem");
      ItemPropertiesSaver saver(XML);
      Writer.Save(*item, saver);
    }
  private:
    QXmlStreamWriter XML;
    const ItemWriter& Writer;
  };

  class ProgressCallbackWrapper : public Playlist::IO::ExportCallback
  {
  public:
    ProgressCallbackWrapper(unsigned total, Playlist::IO::ExportCallback& delegate)
      : Total(total)
      , Delegate(delegate)
      , CurProgress(~0)
      , Canceled()
    {
    }

    virtual void Progress(unsigned items)
    {
      const unsigned newProgress = items * 100 / Total;
      if (CurProgress != newProgress)
      {
        Delegate.Progress(CurProgress = newProgress);
        Canceled = Delegate.IsCanceled();
      }
    }

    virtual bool IsCanceled() const
    {
      return Canceled;
    }
  private:
    const unsigned Total;
    Playlist::IO::ExportCallback& Delegate;
    unsigned CurProgress;
    bool Canceled;
  };
}

namespace Playlist
{
  namespace IO
  {
    Error SaveXSPF(Container::Ptr container, const QString& filename, ExportCallback& cb, ExportFlags flags)
    {
      QFile device(filename);
      if (!device.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
      {
        return Error(THIS_LINE, FromQString(QFile::tr("Cannot create %1 for output").arg(filename)));
      }
      try
      {
        const std::auto_ptr<const ItemWriter> itemWriter = CreateWriter(filename, flags);
        XSPFWriter writer(device, *itemWriter);
        const Parameters::Accessor::Ptr playlistProperties = container->GetProperties();
        const unsigned itemsCount = container->GetItemsCount();
        writer.WriteProperties(*playlistProperties, itemsCount);
        ProgressCallbackWrapper progress(itemsCount, cb);
        writer.WriteItems(*container, progress);
        return Error();
      }
      catch (const Error& e)
      {
        return e;
      }
    }
  }
}

