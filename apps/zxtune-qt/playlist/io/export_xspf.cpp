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
//qt includes
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QXmlStreamWriter>

namespace
{
  const std::string THIS_MODULE("Playlist::IO::XSPF");

  class PropertiesSaver : public Parameters::Visitor
  {
  public:
    PropertiesSaver(QXmlStreamWriter& xml, const Char* tag)
      : XML(xml)
    {
      XML.writeStartElement(tag);
    }

    virtual ~PropertiesSaver()
    {
      XML.writeEndElement();
    }

    virtual void SetIntValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      Log::Debug(THIS_MODULE, "  saving attribute %1%=%2%", name, val);
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      Log::Debug(THIS_MODULE, "  saving attribute %1%='%2%'", name, val);
      if (name == ZXTune::Module::ATTR_FULLPATH)
      {
        XML.writeTextElement(XSPF::ITEM_LOCATION_TAG, ToQString(val));
      }
      else if (name == ZXTune::Module::ATTR_TITLE)
      {
        XML.writeTextElement(XSPF::ITEM_TITLE_TAG, ToQString(val));
      }
      else if (name == ZXTune::Module::ATTR_AUTHOR)
      {
        XML.writeTextElement(XSPF::ITEM_CREATOR_TAG, ToQString(val));
      }
      else if (name == ZXTune::Module::ATTR_COMMENT)
      {
        XML.writeTextElement(XSPF::ITEM_ANNOTATION_TAG, ToQString(val));
      }
    }

    virtual void SetDataValue(const Parameters::NameType& /*name*/, const Parameters::DataType& /*val*/)
    {
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
    }

    void WriteItems(Playitem::Iterator::Ptr iter)
    {
      XML.writeStartElement(XSPF::TRACKLIST_TAG);
      for (; iter->IsValid(); iter->Next())
      {
        const Playitem::Ptr item = iter->Get();
        WriteItem(*item);
      }
      XML.writeEndElement();
    }

    ~XSPFWriter()
    {
      XML.writeEndDocument();
    }
  private:
    void WriteItem(const Playitem& item)
    {
      Log::Debug(THIS_MODULE, "Save playitem '%1%'", item.GetAttributes().GetTitle());
      const ZXTune::Module::Holder::Ptr holder = item.GetModule();
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      const Parameters::Accessor::Ptr props = info->Properties();

      PropertiesSaver saver(XML, XSPF::ITEM_TAG);
      props->Process(saver);
    }
  private:
    QXmlStreamWriter XML;
  };
}

namespace Playlist
{
  namespace IO
  {
    bool SaveXSPF(Container::Ptr container, const class QString& filename)
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

