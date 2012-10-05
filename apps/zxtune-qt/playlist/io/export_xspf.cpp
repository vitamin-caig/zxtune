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
#include <debug_log.h>
#include <error_tools.h>
//library includes
#include <zxtune.h>
#include <core/module_attrs.h>
#include <io/error_codes.h>
#include <sound/sound_parameters.h>
#include <io/text/io.h>
//boost includes
#include <boost/scoped_ptr.hpp>
//qt includes
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
        XML.writeAttribute(QLatin1String(XSPF::APPLICATION_ATTR), QLatin1String(Text::PROGRAM_SITE));
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
      Dbg("  saving item location %1%", location);
      const QUrl url(ToQString(location));
      XML.writeTextElement(QLatin1String(XSPF::ITEM_LOCATION_TAG), DataToQString(url.toEncoded()));
    }

    void SaveModuleProperties(const ZXTune::Module::Information& info, const Parameters::Accessor& props)
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
  private:
    virtual void SetValue(const Parameters::NameType& /*name*/, Parameters::IntType /*val*/)
    {
    }

    virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      const String value = Parameters::ConvertToString(val);
      const QString valStr = ConvertString(value);
      if (name == ZXTune::Module::ATTR_TITLE)
      {
        Dbg("  saving item attribute %1%='%2%'", name.FullPath(), val);
        SaveText(XSPF::ITEM_TITLE_TAG, valStr);
      }
      else if (name == ZXTune::Module::ATTR_AUTHOR)
      {
        Dbg("  saving item attribute %1%='%2%'", name.FullPath(), val);
        SaveText(XSPF::ITEM_CREATOR_TAG, valStr);
      }
      else if (name == ZXTune::Module::ATTR_COMMENT)
      {
        Dbg("  saving item attribute %1%='%2%'", name.FullPath(), val);
        SaveText(XSPF::ITEM_ANNOTATION_TAG, valStr);
      }
    }

    virtual void SetValue(const Parameters::NameType& /*name*/, const Parameters::DataType& /*val*/)
    {
    }
  private:
    void SaveDuration(const ZXTune::Module::Information& info, const Parameters::Accessor& props)
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
        name != ZXTune::Module::ATTR_FULLPATH &&
        name != ZXTune::Module::ATTR_PATH &&
        name != ZXTune::Module::ATTR_FILENAME &&
        name != ZXTune::Module::ATTR_SUBPATH &&
        //skip existing properties
        name != ZXTune::Module::ATTR_AUTHOR &&
        name != ZXTune::Module::ATTR_TITLE &&
        name != ZXTune::Module::ATTR_COMMENT &&
        //skip redundand properties
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

  class XSPFWriter
  {
  public:
    XSPFWriter(QIODevice& device, bool saveAttributes)
      : XML(&device)
      , SaveAttributes(saveAttributes)
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
      saver.SaveModuleLocation(item->GetFullPath());
      const Parameters::Accessor::Ptr adjustedParams = item->GetAdjustedParameters();
      if (const ZXTune::Module::Holder::Ptr holder = SaveAttributes ? item->GetModule() : ZXTune::Module::Holder::Ptr())
      {
        const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
        const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
        saver.SaveModuleProperties(*info, *props);
      }
      else
      {
        saver.SaveStubModuleProperties(*adjustedParams);
      }
      saver.SaveAdjustedParameters(*adjustedParams);
    }
  private:
    QXmlStreamWriter XML;
    const bool SaveAttributes;
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
        return Error(THIS_LINE, ZXTune::IO::ERROR_IO_ERROR, FromQString(QFile::tr("Cannot create %1 for output").arg(filename)));
      }
      try
      {
        XSPFWriter writer(device, 0 != (flags & SAVE_ATTRIBUTES));
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

