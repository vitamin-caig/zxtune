/*
Abstract:
  Components dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "componentsdialog.h"
#include "componentsdialog_ui.h"
#include "componentsdialog_moc.h"
#include "ui/utils.h"
//library includes
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/provider.h>
#include <sound/backend.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/text.h"

namespace
{
  bool IsPlayerPlugin(const ZXTune::Plugin& plugin)
  {
    return ZXTune::CAP_STOR_MODULE == (plugin.Capabilities() & ZXTune::CAP_STORAGE_MASK);
  }

  bool IsAYMPlugin(const ZXTune::Plugin& plugin)
  {
    assert(IsPlayerPlugin(plugin));
    return 0 == ((plugin.Capabilities() & ZXTune::CAP_DEVICE_MASK) & ~ZXTune::CAP_DEV_AYM_MASK);
  }

  bool IsDACPlugin(const ZXTune::Plugin& plugin)
  {
    assert(IsPlayerPlugin(plugin));
    return 0 == ((plugin.Capabilities() & ZXTune::CAP_DEVICE_MASK) & ~ZXTune::CAP_DEV_DAC_MASK);
  }

  bool IsMultitrackPlugin(const ZXTune::Plugin& plugin)
  {
    assert(!IsPlayerPlugin(plugin));
    return ZXTune::CAP_STOR_MULTITRACK == (plugin.Capabilities() & ZXTune::CAP_STOR_MULTITRACK);
  }

  bool IsImplicitPlugin(const ZXTune::Plugin& plugin)
  {
    assert(!IsPlayerPlugin(plugin));
    return ZXTune::CAP_STOR_CONTAINER == (plugin.Capabilities() & ZXTune::CAP_STOR_CONTAINER);
  }

  void AddVersion(const String& version, QTreeWidgetItem& root)
  {
    QString str = ComponentsDialog::tr("Version: ");
    str += ToQString(version);
    new QTreeWidgetItem(&root, QStringList(str));
  }

  void AddCapability(uint_t caps, uint_t mask, QTreeWidgetItem& root, const char* notation)
  {
    if (mask == (caps & mask))
    {
      const QString& title = ComponentsDialog::tr(notation);
      new QTreeWidgetItem(&root, QStringList(title));
    }
  }

  class PluginsTreeHelper
  {
  public:
    explicit PluginsTreeHelper(QTreeWidget& widget)
      : Widget(widget)
      , Players(new QTreeWidgetItem(&Widget, QStringList(ComponentsDialog::tr("Player plugins"))))
      , Containers(new QTreeWidgetItem(&Widget, QStringList(ComponentsDialog::tr("Container plugins"))))
      , Ayms(new QTreeWidgetItem(Players, QStringList(ComponentsDialog::tr("AY/YM"))))
      , Dacs(new QTreeWidgetItem(Players, QStringList(ComponentsDialog::tr("DAC"))))
      , Multitracks(new QTreeWidgetItem(Containers, QStringList(ComponentsDialog::tr("Multitrack"))))
      , Implicits(new QTreeWidgetItem(Containers, QStringList(ComponentsDialog::tr("Implicit"))))
    {
    }

    void AddPlugin(const ZXTune::Plugin& plugin)
    {
      if (IsPlayerPlugin(plugin))
      {
        AddPlayerPlugin(plugin);
      }
      else
      {
        AddStoragePlugin(plugin);
      }
    }
  private:
    void AddPlayerPlugin(const ZXTune::Plugin& plugin)
    {
      if (IsAYMPlugin(plugin))
      {
        AddPlayerPluginItem(plugin, *Ayms);
      }
      else if (IsDACPlugin(plugin))
      {
        AddPlayerPluginItem(plugin, *Dacs);
      }
      else
      {
        assert(!"Unknown player plugin");
      }
    }

    void AddPlayerPluginItem(const ZXTune::Plugin& plugin, QTreeWidgetItem& root)
    {
      assert(IsPlayerPlugin(plugin));
      const String& description = plugin.Description();
      const String& id = plugin.Id();
      const QString& conversionTitle = ComponentsDialog::tr("Conversion targets");

      //root
      QTreeWidgetItem* const pluginItem = new QTreeWidgetItem(&root, QStringList(ToQString(description)));
      String iconPath = Text::TYPEICONS_RESOURCE_PREFIX;
      iconPath += id;
      pluginItem->setIcon(0, QIcon(ToQString(iconPath)));
      //version
      AddVersion(plugin.Version(), *pluginItem);
      //conversion
      if (uint_t convCaps = plugin.Capabilities() & ZXTune::CAP_CONVERSION_MASK)
      {
        QTreeWidgetItem* const conversionItem = new QTreeWidgetItem(pluginItem, QStringList(conversionTitle));
        FillConversionCapabilities(convCaps, *conversionItem);
      }
    }

    void FillConversionCapabilities(uint_t caps, QTreeWidgetItem& root)
    {
      AddCapability(caps, ZXTune::CAP_CONV_RAW, root, QT_TR_NOOP("Raw ripping"));
      AddCapability(caps, ZXTune::CAP_CONV_OUT, root, QT_TR_NOOP("Streamed .out format"));
      AddCapability(caps, ZXTune::CAP_CONV_PSG, root, QT_TR_NOOP("Streamed .psg format"));
      AddCapability(caps, ZXTune::CAP_CONV_YM, root, QT_TR_NOOP("Streamed .ym format"));
      AddCapability(caps, ZXTune::CAP_CONV_ZX50, root, QT_TR_NOOP("Streamed .zx50 format"));
      AddCapability(caps, ZXTune::CAP_CONV_TXT, root, QT_TR_NOOP("Vortex .txt format"));
    }

    void AddStoragePlugin(const ZXTune::Plugin& plugin)
    {
      if (IsMultitrackPlugin(plugin))
      {
        AddContainerPluginItem(plugin, *Multitracks);
      }
      else if (IsImplicitPlugin(plugin))
      {
        AddContainerPluginItem(plugin, *Implicits);
      }
      else
      {
        assert(!"Unknown container plugin");
      }
    }

    void AddContainerPluginItem(const ZXTune::Plugin& plugin, QTreeWidgetItem& root)
    {
      assert(!IsPlayerPlugin(plugin));
      const String& description = plugin.Description();
      const QString& capabilitiesTitle = ComponentsDialog::tr("Capabilities");

      //root
      QTreeWidgetItem* const pluginItem = new QTreeWidgetItem(&root, QStringList(ToQString(description)));
      //version
      AddVersion(plugin.Version(), *pluginItem);
      //capabilities
      const uint_t otherCapsMask = ZXTune::CAP_STORAGE_MASK & ~(ZXTune::CAP_STOR_MULTITRACK | ZXTune::CAP_STOR_CONTAINER);
      if (uint_t caps = plugin.Capabilities() & otherCapsMask)
      {
        QTreeWidgetItem* const capabilitiesItem = new QTreeWidgetItem(pluginItem, QStringList(capabilitiesTitle));
        FillStorageCapabilities(caps, *capabilitiesItem);
      }
    }

    void FillStorageCapabilities(uint_t caps, QTreeWidgetItem& root) const
    {
      AddCapability(caps, ZXTune::CAP_STOR_SCANER, root, QT_TR_NOOP("Data scanner"));
      AddCapability(caps, ZXTune::CAP_STOR_PLAIN, root, QT_TR_NOOP("Plain data structure format"));
    }
  private:
    QTreeWidget& Widget;
    //1st level
    QTreeWidgetItem* const Players;
    QTreeWidgetItem* const Containers;
    //2nd level
    QTreeWidgetItem* const Ayms;
    QTreeWidgetItem* const Dacs;
    QTreeWidgetItem* const Multitracks;
    QTreeWidgetItem* const Implicits;
  };

  bool IsPlaybackBackend(const ZXTune::Sound::BackendInformation& backend)
  {
    return ZXTune::Sound::CAP_TYPE_SYSTEM == (backend.Capabilities() & ZXTune::Sound::CAP_TYPE_MASK);
  }

  bool IsFilesaveBackend(const ZXTune::Sound::BackendInformation& backend)
  {
    return ZXTune::Sound::CAP_TYPE_FILE == (backend.Capabilities() & ZXTune::Sound::CAP_TYPE_MASK);
  }

  bool IsHardwareBackend(const ZXTune::Sound::BackendInformation& backend)
  {
    return ZXTune::Sound::CAP_TYPE_HARDWARE == (backend.Capabilities() & ZXTune::Sound::CAP_TYPE_MASK);
  }

  class BackendsTreeHelper
  {
  public:
    explicit BackendsTreeHelper(QTreeWidget& widget)
      : Widget(widget)
      , Playbacks(new QTreeWidgetItem(&Widget, QStringList(ComponentsDialog::tr("Playback backends"))))
      , Filesaves(new QTreeWidgetItem(&Widget, QStringList(ComponentsDialog::tr("File backends"))))
      , Hardwares(new QTreeWidgetItem(&Widget, QStringList(ComponentsDialog::tr("Hardware backends"))))
      , Others(new QTreeWidgetItem(&Widget, QStringList(ComponentsDialog::tr("Other backends"))))
    {
    }

    void AddBackend(const ZXTune::Sound::BackendInformation& backend)
    {
      if (IsPlaybackBackend(backend))
      {
        assert(!IsFilesaveBackend(backend));
        assert(!IsHardwareBackend(backend));
        AddBackend(*Playbacks, backend);
      }
      else if (IsFilesaveBackend(backend))
      {
        assert(!IsHardwareBackend(backend));
        AddBackend(*Filesaves, backend);
      }
      else if (IsHardwareBackend(backend))
      {
        AddBackend(*Hardwares, backend);
      }
      else
      {
        AddBackend(*Others, backend);
      }
    }
  private:
    void AddBackend(QTreeWidgetItem& root, const ZXTune::Sound::BackendInformation& backend)
    {
      const String& description = backend.Description();
      const QString& featuresTitle = ComponentsDialog::tr("Features");

      //root
      QTreeWidgetItem* const backendItem = new QTreeWidgetItem(&root, QStringList(ToQString(description)));
      //version
      AddVersion(backend.Version(), *backendItem);
      //features
      if (uint_t features = backend.Capabilities() & ZXTune::Sound::CAP_FEAT_MASK)
      {
        QTreeWidgetItem* const featuresItem = new QTreeWidgetItem(backendItem, QStringList(featuresTitle));
        FillBackendFeatures(features, *featuresItem);
      }
    }

    void FillBackendFeatures(uint_t feats, QTreeWidgetItem& root)
    {
      AddCapability(feats, ZXTune::Sound::CAP_FEAT_HWVOLUME, root, QT_TR_NOOP("Hardware volume control"));
    }
  private:
    QTreeWidget& Widget;
    //1st level
    QTreeWidgetItem* const Playbacks;
    QTreeWidgetItem* const Filesaves;
    QTreeWidgetItem* const Hardwares;
    QTreeWidgetItem* const Others;
  };

  class ProvidersTreeHelper
  {
  public:
    explicit ProvidersTreeHelper(QTreeWidget& widget)
      : Widget(widget)
    {
    }

    void AddProvider(const ZXTune::IO::Provider& provider)
    {
      const String& description = provider.Description();

      //root
      QTreeWidgetItem* const providerItem = new QTreeWidgetItem(&Widget, QStringList(ToQString(description)));
      //version
      AddVersion(provider.Version(), *providerItem);
    }
  private:
    QTreeWidget& Widget;
  };

  class ComponentsDialogImpl : public ComponentsDialog
                             , private Ui::ComponentsDialog
  {
  public:
    explicit ComponentsDialogImpl(QWidget* /*parent*/)
    {
      //do not set parent
      setupUi(this);
      FillPluginsTree();
      FillBackendsTree();
      FillProvidersTree();
    }

    virtual void Show()
    {
      exec();
    }
  private:
    void FillPluginsTree()
    {
      PluginsTreeHelper tree(*pluginsTree);

      for (ZXTune::Plugin::Iterator::Ptr plugins = ZXTune::EnumeratePlugins(); plugins->IsValid(); plugins->Next())
      {
        const ZXTune::Plugin::Ptr plugin = plugins->Get();
        tree.AddPlugin(*plugin);
      }
    }

    void FillBackendsTree()
    {
      BackendsTreeHelper tree(*backendsTree);

      for (ZXTune::Sound::BackendCreator::Iterator::Ptr backends = ZXTune::Sound::EnumerateBackends(); backends->IsValid(); backends->Next())
      {
        const ZXTune::Sound::BackendInformation::Ptr backend = backends->Get();
        tree.AddBackend(*backend);
      }
    }

    void FillProvidersTree()
    {
      ProvidersTreeHelper tree(*providersTree);

      for (ZXTune::IO::Provider::Iterator::Ptr providers = ZXTune::IO::EnumerateProviders(); providers->IsValid(); providers->Next())
      {
        const ZXTune::IO::Provider::Ptr provider = providers->Get();
        tree.AddProvider(*provider);
      }
    }
  };
}

ComponentsDialog* ComponentsDialog::Create(QWidget* parent)
{
  return new ComponentsDialogImpl(parent);
}
