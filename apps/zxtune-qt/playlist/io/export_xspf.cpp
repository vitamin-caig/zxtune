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
#include <error_tools.h>
#include <logging.h>
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
  const std::string THIS_MODULE("Playlist::IO::XSPF");

  const unsigned XSPF_VERSION = 1;

  typedef bool (*AttributesFilter)(const Parameters::NameType&);

  QString ConvertString(const String& str)
  {
    return QUrl::toPercentEncoding(str.c_str());
  }

  class ExtendedPropertiesSaver : public Parameters::Visitor
  {
    class StringPropertySaver
    {
    public:
      explicit StringPropertySaver(QXmlStreamWriter& xml)
        : XML(xml)
      {
        XML.writeStartElement(XSPF::EXTENSION_TAG);
        XML.writeAttribute(XSPF::APPLICATION_ATTR, Text::PROGRAM_SITE);
      }

      ~StringPropertySaver()
      {
        XML.writeEndElement();
      }

      void SaveProperty(const Parameters::NameType& name, const String& strVal)
      {
        XML.writeStartElement(XSPF::EXTENDED_PROPERTY_TAG);
        XML.writeAttribute(XSPF::EXTENDED_PROPERTY_NAME_ATTR, ToQString(name));
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

    virtual void SetIntValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Log::Debug(THIS_MODULE, "  saving extended attribute %1%=%2%", name, val);
      SaveProperty(name, val);
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      if (Filter && !Filter(name))
      {
        return;
      }
      Log::Debug(THIS_MODULE, "  saving extended attribute %1%='%2%'", name, val);
      SaveProperty(name, val);
    }

    virtual void SetDataValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      if (Filter && !Filter(name))
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
      XML.writeStartElement(XSPF::ITEM_TAG);
    }

    virtual ~ItemPropertiesSaver()
    {
      XML.writeEndElement();
    }

    void SaveModuleLocation(const String& location)
    {
      Log::Debug(THIS_MODULE, "  saving item location %1%", location);
      const QUrl url(ToQString(location));
      XML.writeTextElement(XSPF::ITEM_LOCATION_TAG, url.toEncoded());
    }

    void SaveModuleProperties(const ZXTune::Module::Information& info, const Parameters::Accessor& props)
    {
      //save common properties
      Log::Debug(THIS_MODULE, " Save basic properties");
      props.Process(*this);
      SaveDuration(info, props);
      Log::Debug(THIS_MODULE, " Save extended properties");
      SaveExtendedProperties(props);
    }

    void SaveStubModuleProperties(const Parameters::Accessor& props)
    {
      Log::Debug(THIS_MODULE, " Save basic stub properties");
      props.Process(*this);
      Log::Debug(THIS_MODULE, " Save stub extended properties");
      SaveExtendedProperties(props);
    }

    void SaveAdjustedParameters(const Parameters::Accessor& params)
    {
      Log::Debug(THIS_MODULE, " Save adjusted parameters");
      ExtendedPropertiesSaver saver(XML, &KeepOnlyParameters);
      params.Process(saver);
    }
  private:
    virtual void SetIntValue(const Parameters::NameType& /*name*/, Parameters::IntType /*val*/)
    {
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      const String value = Parameters::ConvertToString(val);
      const QString valStr = ConvertString(value);
      if (name == ZXTune::Module::ATTR_TITLE)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        SaveText(XSPF::ITEM_TITLE_TAG, valStr);
      }
      else if (name == ZXTune::Module::ATTR_AUTHOR)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        SaveText(XSPF::ITEM_CREATOR_TAG, valStr);
      }
      else if (name == ZXTune::Module::ATTR_COMMENT)
      {
        Log::Debug(THIS_MODULE, "  saving item attribute %1%='%2%'", name, val);
        SaveText(XSPF::ITEM_ANNOTATION_TAG, valStr);
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

    void SaveText(const Char* tag, const QString& value)
    {
      XML.writeTextElement(tag, value);
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
        name != ZXTune::Module::ATTR_WARNINGS_COUNT &&
        name != ZXTune::Module::ATTR_WARNINGS &&
        //skip all the parameters
        !IsParameter(name)
      ;
    }

    static bool IsParameter(const Parameters::NameType& name)
    {
      return Parameters::NameType::npos != name.find(Parameters::NAMESPACE_DELIMITER);
    }

    static bool KeepOnlyParameters(const Parameters::NameType& name)
    {
      return 0 == name.find(Parameters::ZXTune::PREFIX) &&
             0 != name.find(Playlist::ATTRIBUTES_PREFIX);
    }

    void SaveExtendedProperties(const Parameters::Accessor& props)
    {
      ExtendedPropertiesSaver saver(XML, &KeepExtendedProperties);
      props.Process(saver);
    }
  private:
    QXmlStreamWriter& XML;
  };

  class CallbackAdapter : public Playlist::Item::Callback
  {
  public:
    CallbackAdapter(Playlist::Item::Callback& delegate, Playlist::IO::ExportCallback& cb)
      : Delegate(delegate)
      , Report(cb)
      , DoneItems()
    {
    }

    virtual void OnItem(Playlist::Item::Data::Ptr data)
    {
      Delegate.OnItem(data);
      Report.Progress(++DoneItems);
    }
  private:
    Playlist::Item::Callback& Delegate;
    Playlist::IO::ExportCallback& Report;
    unsigned DoneItems;
  };

  class XSPFWriter : private Playlist::Item::Callback
  {
  public:
    XSPFWriter(QIODevice& device, bool saveAttributes)
      : XML(&device)
      , SaveAttributes(saveAttributes)
    {
      XML.setAutoFormatting(true);
      XML.setAutoFormattingIndent(2);
      XML.writeStartDocument();
      XML.writeStartElement(XSPF::ROOT_TAG);
      XML.writeAttribute(XSPF::VERSION_ATTR, XSPF::VERSION_VALUE);
      XML.writeAttribute(XSPF::XMLNS_ATTR, XSPF::XMLNS_VALUE);
    }

    void WriteProperties(const Parameters::Accessor& props, uint_t items)
    {
      ExtendedPropertiesSaver saver(XML);
      props.Process(saver);
      saver.SetIntValue(Playlist::ATTRIBUTE_VERSION, XSPF_VERSION);
      saver.SetIntValue(Playlist::ATTRIBUTE_ITEMS, items);
    }

    void WriteItems(const Playlist::IO::Container& container, Playlist::IO::ExportCallback& cb)
    {
      XML.writeStartElement(XSPF::TRACKLIST_TAG);
      CallbackAdapter adapter(*this, cb);
      container.ForAllItems(adapter);
      XML.writeEndElement();
    }

    ~XSPFWriter()
    {
      XML.writeEndDocument();
    }
  private:
    virtual void OnItem(Playlist::Item::Data::Ptr item)
    {
      Log::Debug(THIS_MODULE, "Save playitem");
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
        return MakeFormattedError(THIS_LINE, ZXTune::IO::ERROR_IO_ERROR, Text::IO_ERROR_NOT_OPENED, FromQString(filename));
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

