/**
 *
 * @file
 *
 * @brief Implementation of .xspf exporting
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "container.h"
#include "export.h"
#include "tags/xspf.h"
#include "ui/utils.h"
// common includes
#include <error_tools.h>
#include <string_view.h>
// library includes
#include <core/plugins/archives/zdata_supp.h>
#include <debug/log.h>
#include <module/attributes.h>
#include <parameters/convert.h>
#include <sound/sound_parameters.h>
#include <zxtune.h>
// std includes
#include <memory>
#include <set>
// qt includes
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QXmlStreamWriter>

namespace
{
  const Debug::Stream Dbg("Playlist::IO::XSPF");

  const QLatin1String ENDL("\n");

  const unsigned XSPF_VERSION = 1;

  using AttributesFilter = bool (*)(Parameters::Identifier);

  QString DataToQString(const QByteArray& data)
  {
    return QString::fromLatin1(data.data(), data.size());
  }

  QString ConvertString(StringView str)
  {
    const QByteArray data = QUrl::toPercentEncoding(ToQString(str));
    return DataToQString(data);
  }

  class ElementHelper
  {
  public:
    ElementHelper(QXmlStreamWriter& xml, const Char* tagName)
      : Xml(xml)
    {
      Xml.writeStartElement(QLatin1String(tagName));
    }

    ~ElementHelper()
    {
      Xml.writeEndElement();
    }

    ElementHelper& Attribute(const Char* name, const QString& value)
    {
      Xml.writeAttribute(QLatin1String(name), value);
      return *this;
    }

    ElementHelper& Text(const QString& str)
    {
      Xml.writeCharacters(str);
      return *this;
    }

    ElementHelper& Text(const Char* name, const QString& str)
    {
      Xml.writeTextElement(QLatin1String(name), str);
      return *this;
    }

    ElementHelper& CData(const QString& str)
    {
      Xml.writeCDATA(str);
      return *this;
    }

    ElementHelper Subtag(const Char* tagName)
    {
      return {Xml, tagName};
    }

  private:
    QXmlStreamWriter& Xml;
  };

  class ExtendedPropertiesSaver : public Parameters::Visitor
  {
    class StringPropertySaver
    {
    public:
      explicit StringPropertySaver(QXmlStreamWriter& xml)
        : Extension(xml, XSPF::EXTENSION_TAG)
      {
        Extension.Attribute(XSPF::APPLICATION_ATTR, Playlist::APPLICATION_ID);
      }

      void SaveProperty(StringView name, StringView strVal)
      {
        Extension.Subtag(XSPF::EXTENDED_PROPERTY_TAG)
            .Attribute(XSPF::EXTENDED_PROPERTY_NAME_ATTR, ToQString(name))
            .Text(ConvertString(strVal));
      }

    private:
      ElementHelper Extension;
    };

  public:
    ExtendedPropertiesSaver(QXmlStreamWriter& xml, AttributesFilter filter = nullptr)
      : XML(xml)
      , Filter(filter)
    {}

    void SetValue(Parameters::Identifier name, Parameters::IntType val) override
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Dbg("  saving extended attribute {}={}", static_cast<StringView>(name), val);
      SaveProperty(name, val);
    }

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Dbg("  saving extended attribute {}='{}'", static_cast<StringView>(name), val);
      SaveProperty(name, val);
    }

    void SetValue(Parameters::Identifier name, Binary::View val) override
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Dbg("  saving extended attribute {}=data({})", static_cast<StringView>(name), val.Size());
      SaveProperty(name, val);
    }

  private:
    template<class T>
    void SaveProperty(Parameters::Identifier name, T value)
    {
      if (!Saver)
      {
        Saver = std::make_unique<StringPropertySaver>(XML);
      }
      const String strVal = Parameters::ConvertToString(value);
      Saver->SaveProperty(name, strVal);
    }

  private:
    QXmlStreamWriter& XML;
    const AttributesFilter Filter;
    std::unique_ptr<StringPropertySaver> Saver;
  };

  class ItemPropertiesSaver : private Parameters::Visitor
  {
  public:
    explicit ItemPropertiesSaver(QXmlStreamWriter& xml)
      : XML(xml)
      , Element(xml, XSPF::ITEM_TAG)
    {}

    void SaveModuleLocation(StringView location)
    {
      Dbg("  saving absolute item location {}", location);
      SaveModuleLocation(ToQString(location));
    }

    void SaveModuleLocation(StringView location, const QDir& root)
    {
      Dbg("  saving relative item location {}", location);
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
      // save common properties
      Dbg(" Save basic properties");
      props.Process(*this);
      SaveDuration(info);
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

    void SaveData(Binary::View content)
    {
      Dbg(" Save content");
      Element.Text(ENDL);
      Element.CData(QString::fromLatin1(static_cast<const char*>(content.Start()), content.Size()));
      Element.Text(ENDL);
    }

  private:
    void SaveModuleLocation(const QString& location)
    {
      Element.Text(XSPF::ITEM_LOCATION_TAG, DataToQString(QUrl(location).toEncoded()));
    }

    void SetValue(Parameters::Identifier /*name*/, Parameters::IntType /*val*/) override {}

    void SetValue(Parameters::Identifier name, StringView val) override
    {
      const String value = Parameters::ConvertToString(val);
      const QString valStr = ConvertString(value);
      if (name == Module::ATTR_TITLE)
      {
        Dbg("  saving item attribute {}='{}'", static_cast<StringView>(name), val);
        Element.Text(XSPF::ITEM_TITLE_TAG, valStr);
      }
      else if (name == Module::ATTR_AUTHOR)
      {
        Dbg("  saving item attribute {}='{}'", static_cast<StringView>(name), val);
        Element.Text(XSPF::ITEM_CREATOR_TAG, valStr);
      }
      else if (name == Module::ATTR_COMMENT)
      {
        Dbg("  saving item attribute {}='{}'", static_cast<StringView>(name), val);
        Element.Text(XSPF::ITEM_ANNOTATION_TAG, valStr);
      }
    }

    void SetValue(Parameters::Identifier /*name*/, Binary::View /*val*/) override {}

  private:
    void SaveDuration(const Module::Information& info)
    {
      const uint64_t msecDuration = info.Duration().CastTo<Time::Millisecond>().Get();
      Dbg("  saving item attribute Duration={}", msecDuration);
      Element.Text(XSPF::ITEM_DURATION_TAG, QString::number(msecDuration));
    }

    static bool KeepExtendedProperties(Parameters::Identifier name)
    {
      return
          // skip path-related properties
          name != Module::ATTR_FULLPATH && name != Module::ATTR_PATH && name != Module::ATTR_FILENAME
          && name != Module::ATTR_EXTENSION && name != Module::ATTR_SUBPATH &&
          // skip existing properties
          name != Module::ATTR_AUTHOR && name != Module::ATTR_TITLE && name != Module::ATTR_COMMENT &&
          // skip redundand properties
          name != Module::ATTR_STRINGS && name != Module::ATTR_PICTURE &&
          // skip all the parameters
          !name.IsPath();
    }

    static bool KeepOnlyParameters(Parameters::Identifier attrName)
    {
      return !attrName.RelativeTo(Parameters::ZXTune::PREFIX).IsEmpty()
             && attrName.RelativeTo(Playlist::ATTRIBUTES_PREFIX).IsEmpty();
    }

    void SaveExtendedProperties(const Parameters::Accessor& props)
    {
      ExtendedPropertiesSaver saver(XML, &KeepExtendedProperties);
      props.Process(saver);
    }

  private:
    QXmlStreamWriter& XML;
    ElementHelper Element;
  };

  class ItemWriter
  {
  public:
    virtual ~ItemWriter() = default;

    virtual void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const = 0;
  };

  class ItemFullLocationWriter : public ItemWriter
  {
  public:
    ItemFullLocationWriter() = default;

    void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const override
    {
      saver.SaveModuleLocation(item.GetFullPath());
    }
  };

  class ItemRelativeLocationWriter : public ItemWriter
  {
  public:
    explicit ItemRelativeLocationWriter(const QString& dirName)
      : Root(dirName)
    {}

    void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const override
    {
      saver.SaveModuleLocation(item.GetFullPath(), Root);
    }

  private:
    const QDir Root;
  };

  class ItemContentLocationWriter : public ItemWriter
  {
  public:
    ItemContentLocationWriter() = default;

    void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const override
    {
      if (const Binary::Data::Ptr rawContent = item.GetModuleData())
      {
        const ZXTune::DataLocation::Ptr container = ZXTune::BuildZdataContainer(*rawContent);
        const String id = container->GetPath()->AsString();
        saver.SaveModuleLocation(XSPF::EMBEDDED_PREFIX + id);
        if (Ids.insert(id).second)
        {
          saver.SaveData(*container->GetData());
        }
        else
        {
          Dbg("Use already stored data for id={}", id);
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
    ItemShortPropertiesWriter() = default;

    void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const override
    {
      const Parameters::Accessor::Ptr adjustedParams = item.GetAdjustedParameters();
      saver.SaveStubModuleProperties(*adjustedParams);
      saver.SaveAdjustedParameters(*adjustedParams);
    }
  };

  class ItemFullPropertiesWriter : public ItemWriter
  {
  public:
    ItemFullPropertiesWriter() = default;

    void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const override
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
    ItemCompositeWriter(std::unique_ptr<ItemWriter> loc, std::unique_ptr<ItemWriter> props)
      : Location(std::move(loc))
      , Properties(std::move(props))
    {}

    void Save(const Playlist::Item::Data& item, ItemPropertiesSaver& saver) const override
    {
      Location->Save(item, saver);
      Properties->Save(item, saver);
    }

  private:
    const std::unique_ptr<ItemWriter> Location;
    const std::unique_ptr<ItemWriter> Properties;
  };

  std::unique_ptr<const ItemWriter> CreateWriter(const QString& filename, Playlist::IO::ExportFlags flags)
  {
    std::unique_ptr<ItemWriter> location;
    std::unique_ptr<ItemWriter> props;
    if (0 != (flags & Playlist::IO::SAVE_CONTENT))
    {
      location = std::make_unique<ItemContentLocationWriter>();
      props = std::make_unique<ItemShortPropertiesWriter>();
    }
    else
    {
      if (0 != (flags & Playlist::IO::SAVE_ATTRIBUTES))
      {
        props = std::make_unique<ItemFullPropertiesWriter>();
      }
      else
      {
        props = std::make_unique<ItemShortPropertiesWriter>();
      }
      if (0 != (flags & Playlist::IO::RELATIVE_PATHS))
      {
        location = std::make_unique<ItemRelativeLocationWriter>(QFileInfo(filename).absolutePath());
      }
      else
      {
        location = std::make_unique<ItemFullLocationWriter>();
      }
    }
    return std::unique_ptr<const ItemWriter>(new ItemCompositeWriter(std::move(location), std::move(props)));
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

    void WriteItems(const Playlist::IO::Container& container, Log::ProgressCallback& cb)
    {
      const uint64_t PERCENTS = 100;
      const ElementHelper tracklist(XML, XSPF::TRACKLIST_TAG);
      const uint_t totalItems = container.GetItemsCount();
      uint_t doneItems = 0;
      for (Playlist::Item::Collection::Ptr const items = container.GetItems(); items->IsValid(); items->Next())
      {
        const Playlist::Item::Data::Ptr item = items->Get();
        WriteItem(*item);
        cb.OnProgress((PERCENTS * ++doneItems / totalItems));
      }
    }

    ~XSPFWriter()
    {
      XML.writeEndDocument();
    }

  private:
    void WriteItem(const Playlist::Item::Data& item)
    {
      Dbg("Save playitem");
      ItemPropertiesSaver saver(XML);
      Writer.Save(item, saver);
    }

  private:
    QXmlStreamWriter XML;
    const ItemWriter& Writer;
  };
}  // namespace

namespace Playlist::IO
{
  void SaveXSPF(const Container& container, const QString& filename, Log::ProgressCallback& cb, ExportFlags flags)
  {
    QFile device(filename);
    if (!device.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
      throw Error(THIS_LINE, FromQString(QFile::tr("Cannot create %1 for output").arg(filename)));
    }
    const std::unique_ptr<const ItemWriter> itemWriter = CreateWriter(filename, flags);
    XSPFWriter writer(device, *itemWriter);
    const Parameters::Accessor::Ptr playlistProperties = container.GetProperties();
    writer.WriteProperties(*playlistProperties, container.GetItemsCount());
    writer.WriteItems(container, cb);
  }
}  // namespace Playlist::IO
