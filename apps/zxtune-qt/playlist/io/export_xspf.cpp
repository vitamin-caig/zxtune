/*
Abstract:
  Playlist to .xspf export implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "export.h"
#include "tags/xspf.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
//library includes
#include <core/module_attrs.h>
#include <sound/sound_parameters.h>
//qt includes
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QXmlStreamWriter>
//text includes
#include "text/text.h"

namespace
{
  const std::string THIS_MODULE("Playlist::IO::XSPF");

  typedef bool (*AttributesFilter)(const Parameters::NameType&);

  class ExtendedPropertiesSaver : public Parameters::Visitor
  {
  public:
    explicit ExtendedPropertiesSaver(QXmlStreamWriter& xml, AttributesFilter filter = 0)
      : XML(xml)
      , Filter(filter)
    {
      XML.writeStartElement(XSPF::EXTENSION_TAG);
      XML.writeAttribute(XSPF::APPLICATION_ATTR, Text::PROGRAM_SITE);
    }

    virtual ~ExtendedPropertiesSaver()
    {
      XML.writeEndElement();
    }

    virtual void SetIntValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      if (Filter && Filter(name))
      {
        return;
      }
      Log::Debug(THIS_MODULE, "  saving extended attribute %1%=%2%", name, val);
      SaveProperty(name, val);
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      if (Filter && Filter(name))
      {
        return;
      }
      Log::Debug(THIS_MODULE, "  saving extended attribute %1%='%2%'", name, val);
      SaveProperty(name, val);
    }

    virtual void SetDataValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      if (Filter && Filter(name))
      {
        return;
      }
      Log::Debug(THIS_MODULE, "  saving extended attribute %1%=data(%2%)", name, val.size());
      SaveProperty(name, val);
    }
  private:
    template<class T>
    void SaveProperty(const Parameters::NameType& name, const T& value)
    {
      const String strVal = Parameters::ConvertToString(value);
      XML.writeStartElement(XSPF::EXTENDED_PROPERTY_TAG);
      XML.writeAttribute(XSPF::EXTENDED_PROPERTY_NAME_ATTR, ToQString(name));
      XML.writeCharacters(ToQString(strVal));
      XML.writeEndElement();
    }
  private:
    QXmlStreamWriter& XML;
    const AttributesFilter Filter;
  };

  class ItemPropertiesSaver : private Parameters::Visitor
  {
  public:
    explicit ItemPropertiesSaver(QXmlStreamWriter& xml)
      : XML(xml)
    {
      XML.writeStartElement(XSPF::ITEM_TAG);
    }

    virtual ~ItemPropertiesSaver()
    {
      XML.writeEndElement();
    }

    void Process(const ZXTune::Module::Information& info)
    {
      //save common properties
      const Parameters::Accessor::Ptr props = info.Properties();
      props->Process(*this);
      SaveDuration(info, *props);
      SaveExtendedProperties(*props);
    }

  private:
    virtual void SetIntValue(const Parameters::NameType& /*name*/, Parameters::IntType /*val*/)
    {
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      const String value = Parameters::ConvertToString(val);
      const QString valStr = ToQString(value);
      if (name == ZXTune::Module::ATTR_FULLPATH)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        const QUrl url(valStr);
        XML.writeTextElement(XSPF::ITEM_LOCATION_TAG, url.toEncoded());
      }
      else if (name == ZXTune::Module::ATTR_TITLE)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        XML.writeTextElement(XSPF::ITEM_TITLE_TAG, valStr);
      }
      else if (name == ZXTune::Module::ATTR_AUTHOR)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        XML.writeTextElement(XSPF::ITEM_CREATOR_TAG, valStr);
      }
      else if (name == ZXTune::Module::ATTR_COMMENT)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        XML.writeTextElement(XSPF::ITEM_ANNOTATION_TAG, valStr);
      }
    }

    virtual void SetDataValue(const Parameters::NameType& /*name*/, const Parameters::DataType& /*val*/)
    {
    }
  private:
    void SaveDuration(const ZXTune::Module::Information& info, const Parameters::Accessor& props)
    {
      Parameters::IntType frameDuration = Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT;
      props.FindIntValue(Parameters::ZXTune::Sound::FRAMEDURATION, frameDuration);
      const uint64_t msecDuration = info.FramesCount() * frameDuration / 1000;
      Log::Debug(THIS_MODULE, "  saving item attribute Duration=%1%", msecDuration);
      XML.writeTextElement(XSPF::ITEM_DURATION_TAG, QString::number(msecDuration));
    }

    static bool FilterExtendedProperties(const Parameters::NameType& name)
    {
      return 
        //skip path-related properties
        name == ZXTune::Module::ATTR_FULLPATH ||
        name == ZXTune::Module::ATTR_PATH ||
        name == ZXTune::Module::ATTR_FILENAME ||
        name == ZXTune::Module::ATTR_SUBPATH ||
        //skip existing properties
        name == ZXTune::Module::ATTR_AUTHOR ||
        name == ZXTune::Module::ATTR_TITLE ||
        name == ZXTune::Module::ATTR_COMMENT
      ;
    }

    void SaveExtendedProperties(const Parameters::Accessor& props)
    {
      ExtendedPropertiesSaver saver(XML, &FilterExtendedProperties);
      props.Process(saver);
    }
  private:
    QXmlStreamWriter& XML;
  };

  class XSPFWriter
  {
  public:
    explicit XSPFWriter(QIODevice& device)
      : XML(&device)
    {
      XML.setAutoFormatting(true);
      XML.setAutoFormattingIndent(2);
      XML.writeStartDocument();
      XML.writeStartElement(XSPF::ROOT_TAG);
      XML.writeAttribute(XSPF::VERSION_ATTR, XSPF::VERSION_VALUE);
      XML.writeAttribute(XSPF::XMLNS_ATTR, XSPF::XMLNS_VALUE);
    }

    void WriteProperties(const Parameters::Accessor& props)
    {
      ExtendedPropertiesSaver saver(XML);
      props.Process(saver);
    }

    void WriteItems(Playlist::Item::Data::Iterator::Ptr iter)
    {
      XML.writeStartElement(XSPF::TRACKLIST_TAG);
      for (; iter->IsValid(); iter->Next())
      {
        const Playlist::Item::Data::Ptr item = iter->Get();
        WriteItem(*item);
      }
      XML.writeEndElement();
    }

    ~XSPFWriter()
    {
      XML.writeEndDocument();
    }
  private:
    void WriteItem(const Playlist::Item::Data& item)
    {
      Log::Debug(THIS_MODULE, "Save playitem");
      const ZXTune::Module::Holder::Ptr holder = item.GetModule();
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();

      ItemPropertiesSaver saver(XML);
      saver.Process(*info);
    }
  private:
    QXmlStreamWriter XML;
  };
}

namespace Playlist
{
  namespace IO
  {
    bool SaveXSPF(Container::Ptr container, const QString& filename)
    {
      QFile device(filename);
      if (!device.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
      {
        return false;
      }
      XSPFWriter writer(device);
      writer.WriteProperties(*container->GetProperties());
      writer.WriteItems(container->GetItems());
      return true;
    }
  }
}

