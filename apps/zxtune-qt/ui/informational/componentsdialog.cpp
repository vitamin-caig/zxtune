/**
* 
* @file
*
* @brief Components dialog implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "componentsdialog.h"
#include "componentsdialog.ui.h"
#include "ui/utils.h"
#include "supp/options.h"
//library includes
#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <io/provider.h>
#include <sound/backend_attrs.h>
#include <sound/service.h>
#include <strings/format.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QDialog>
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
    return 0 != (plugin.Capabilities() & (ZXTune::CAP_DEV_AY38910 | ZXTune::CAP_DEV_TURBOSOUND));
  }

  bool IsDACPlugin(const ZXTune::Plugin& plugin)
  {
    assert(IsPlayerPlugin(plugin));
    return 0 != (plugin.Capabilities() & (ZXTune::CAP_DEV_DAC | ZXTune::CAP_DEV_BEEPER));
  }

  bool IsFMPlugin(const ZXTune::Plugin& plugin)
  {
    assert(IsPlayerPlugin(plugin));
    return 0 != (plugin.Capabilities() & (ZXTune::CAP_DEV_YM2203 | ZXTune::CAP_DEV_TURBOFM));
  }

  bool IsSAAPlugin(const ZXTune::Plugin& plugin)
  {
    assert(IsPlayerPlugin(plugin));
    return 0 != (plugin.Capabilities() & ZXTune::CAP_DEV_SAA1099);
  }

  bool IsMultitrackPlugin(const ZXTune::Plugin& plugin)
  {
    assert(!IsPlayerPlugin(plugin));
    return ZXTune::CAP_STOR_MULTITRACK == (plugin.Capabilities() & ZXTune::CAP_STOR_MULTITRACK);
  }

  bool IsArchivePlugin(const ZXTune::Plugin& plugin)
  {
    assert(!IsPlayerPlugin(plugin));
    return ZXTune::CAP_STOR_CONTAINER == (plugin.Capabilities() & ZXTune::CAP_STOR_CONTAINER);
  }

  QString Translate(const char* msg)
  {
    return QApplication::translate("ComponentsDialog", msg, 0, QApplication::UnicodeUTF8);
  }

  template<class T>
  QTreeWidgetItem* CreateTreeWidgetItem(T* parent, const char* title)
  {
    return new QTreeWidgetItem(parent, QStringList(Translate(title)));
  }

  void AddCapability(uint_t caps, uint_t mask, QTreeWidgetItem& root, const char* notation)
  {
    if (mask == (caps & mask))
    {
      CreateTreeWidgetItem(&root, notation);
    }
  }

  class PluginsTreeHelper
  {
  public:
    explicit PluginsTreeHelper(QTreeWidget& widget)
      : Widget(widget)
      , Players(CreateTreeWidgetItem(&Widget, QT_TRANSLATE_NOOP("ComponentsDialog", "Player plugins")))
      , Containers(CreateTreeWidgetItem(&Widget, QT_TRANSLATE_NOOP("ComponentsDialog", "Container plugins")))
      , Ayms(CreateTreeWidgetItem(Players, "AY/YM"))
      , Dacs(CreateTreeWidgetItem(Players, "DAC"))
      , Fms(CreateTreeWidgetItem(Players, "FM"))
      , Saas(CreateTreeWidgetItem(Players, "SAA"))
      , Multitracks(CreateTreeWidgetItem(Containers, QT_TRANSLATE_NOOP("ComponentsDialog", "Multitrack")))
      , Archives(CreateTreeWidgetItem(Containers, QT_TRANSLATE_NOOP("ComponentsDialog", "Archive")))
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
      else if (IsFMPlugin(plugin))
      {
        AddPlayerPluginItem(plugin, *Fms);
      }
      else if (IsSAAPlugin(plugin))
      {
        AddPlayerPluginItem(plugin, *Saas);
      }
      else
      {
        assert(!"Unknown player plugin");
      }
    }

    void AddPlayerPluginItem(const ZXTune::Plugin& plugin, QTreeWidgetItem& root)
    {
      assert(IsPlayerPlugin(plugin));

      //root
      const String& title = Strings::Format("[%s] %s", plugin.Id(), plugin.Description());
      QTreeWidgetItem* const pluginItem = new QTreeWidgetItem(&root, QStringList(ToQString(title)));
      //conversion
      if (uint_t convCaps = plugin.Capabilities() & ZXTune::CAP_CONVERSION_MASK)
      {
        QTreeWidgetItem* const conversionItem = CreateTreeWidgetItem(pluginItem, QT_TRANSLATE_NOOP("ComponentsDialog", "Conversion targets"));
        FillConversionCapabilities(convCaps, *conversionItem);
      }
    }

    void FillConversionCapabilities(uint_t caps, QTreeWidgetItem& root)
    {
      AddCapability(caps, ZXTune::CAP_CONV_RAW, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Raw ripping"));
      AddCapability(caps, ZXTune::CAP_CONV_OUT, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Streamed .out format"));
      AddCapability(caps, ZXTune::CAP_CONV_PSG, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Streamed .psg format"));
      AddCapability(caps, ZXTune::CAP_CONV_YM, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Streamed .ym format"));
      AddCapability(caps, ZXTune::CAP_CONV_ZX50, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Streamed .zx50 format"));
      AddCapability(caps, ZXTune::CAP_CONV_TXT, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Vortex .txt format"));
      AddCapability(caps, ZXTune::CAP_CONV_AYDUMP, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Raw aydump format"));
      AddCapability(caps, ZXTune::CAP_CONV_FYM, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Compressed .fym format"));
    }

    void AddStoragePlugin(const ZXTune::Plugin& plugin)
    {
      if (IsMultitrackPlugin(plugin))
      {
        AddContainerPluginItem(plugin, *Multitracks);
      }
      else if (IsArchivePlugin(plugin))
      {
        AddContainerPluginItem(plugin, *Archives);
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

      //root
      QTreeWidgetItem* const pluginItem = new QTreeWidgetItem(&root, QStringList(ToQString(description)));
      //capabilities
      const uint_t otherCapsMask = ZXTune::CAP_STORAGE_MASK & ~(ZXTune::CAP_STOR_MULTITRACK | ZXTune::CAP_STOR_CONTAINER);
      if (uint_t caps = plugin.Capabilities() & otherCapsMask)
      {
        QTreeWidgetItem* const capabilitiesItem = CreateTreeWidgetItem(pluginItem, QT_TRANSLATE_NOOP("ComponentsDialog", "Capabilities"));
        FillStorageCapabilities(caps, *capabilitiesItem);
      }
    }

    void FillStorageCapabilities(uint_t caps, QTreeWidgetItem& root) const
    {
      AddCapability(caps, ZXTune::CAP_STOR_SCANER, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Data scanner"));
      AddCapability(caps, ZXTune::CAP_STOR_PLAIN, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Plain data structure format"));
      AddCapability(caps, ZXTune::CAP_STOR_DIRS, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Directories support"));
    }
  private:
    QTreeWidget& Widget;
    //1st level
    QTreeWidgetItem* const Players;
    QTreeWidgetItem* const Containers;
    //2nd level
    QTreeWidgetItem* const Ayms;
    QTreeWidgetItem* const Dacs;
    QTreeWidgetItem* const Fms;
    QTreeWidgetItem* const Saas;
    QTreeWidgetItem* const Multitracks;
    QTreeWidgetItem* const Archives;
  };

  bool IsPlaybackBackend(const Sound::BackendInformation& backend)
  {
    return Sound::CAP_TYPE_SYSTEM == (backend.Capabilities() & Sound::CAP_TYPE_MASK);
  }

  bool IsFilesaveBackend(const Sound::BackendInformation& backend)
  {
    return Sound::CAP_TYPE_FILE == (backend.Capabilities() & Sound::CAP_TYPE_MASK);
  }

  bool IsHardwareBackend(const Sound::BackendInformation& backend)
  {
    return Sound::CAP_TYPE_HARDWARE == (backend.Capabilities() & Sound::CAP_TYPE_MASK);
  }

  template<class T>
  QTreeWidgetItem* CreateRootItem(T& root, const String& description, const Error& status)
  {
    QTreeWidgetItem* const item = new QTreeWidgetItem(&root, QStringList(ToQString(description)));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if (status)
    {
      item->setCheckState(0, Qt::Unchecked);
      item->setToolTip(0, ToQString(status.ToString()));
    }
    else
    {
      item->setCheckState(0, Qt::Checked);
    }
    return item;
  }

  class BackendsTreeHelper
  {
  public:
    explicit BackendsTreeHelper(QTreeWidget& widget)
      : Widget(widget)
      , Playbacks(CreateTreeWidgetItem(&Widget, QT_TRANSLATE_NOOP("ComponentsDialog", "Playback backends")))
      , Filesaves(CreateTreeWidgetItem(&Widget, QT_TRANSLATE_NOOP("ComponentsDialog", "File backends")))
      , Hardwares(CreateTreeWidgetItem(&Widget, QT_TRANSLATE_NOOP("ComponentsDialog", "Hardware backends")))
      , Others(CreateTreeWidgetItem(&Widget, QT_TRANSLATE_NOOP("ComponentsDialog", "Other backends")))
    {
    }

    void AddBackend(const Sound::BackendInformation& backend)
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
    void AddBackend(QTreeWidgetItem& root, const Sound::BackendInformation& backend)
    {
      //root
      QTreeWidgetItem* const backendItem = CreateRootItem(root, backend.Description(), backend.Status());
      //features
      if (uint_t features = backend.Capabilities() & Sound::CAP_FEAT_MASK)
      {
        QTreeWidgetItem* const featuresItem = CreateTreeWidgetItem(backendItem, QT_TRANSLATE_NOOP("ComponentsDialog", "Features"));
        FillBackendFeatures(features, *featuresItem);
      }
    }

    void FillBackendFeatures(uint_t feats, QTreeWidgetItem& root)
    {
      AddCapability(feats, Sound::CAP_FEAT_HWVOLUME, root, QT_TRANSLATE_NOOP("ComponentsDialog", "Hardware volume control"));
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

    void AddProvider(const IO::Provider& provider)
    {
      //root
      CreateRootItem(Widget, provider.Description(), provider.Status());
    }
  private:
    QTreeWidget& Widget;
  };

  class ComponentsDialog : public QDialog
                         , private Ui::ComponentsDialog
  {
  public:
    explicit ComponentsDialog(QWidget& parent)
      : QDialog(&parent)
    {
      setupUi(this);
      FillPluginsTree();
      FillBackendsTree();
      FillProvidersTree();
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
      const Sound::Service::Ptr svc = Sound::CreateGlobalService(GlobalOptions::Instance().Get());
      for (Sound::BackendInformation::Iterator::Ptr backends = svc->EnumerateBackends(); backends->IsValid(); backends->Next())
      {
        const Sound::BackendInformation::Ptr backend = backends->Get();
        tree.AddBackend(*backend);
      }
    }

    void FillProvidersTree()
    {
      ProvidersTreeHelper tree(*providersTree);

      for (IO::Provider::Iterator::Ptr providers = IO::EnumerateProviders(); providers->IsValid(); providers->Next())
      {
        const IO::Provider::Ptr provider = providers->Get();
        tree.AddProvider(*provider);
      }
    }
  };
}

namespace UI
{
  void ShowComponentsInformation(QWidget& parent)
  {
    ComponentsDialog dialog(parent);
    dialog.exec();
  }
}
