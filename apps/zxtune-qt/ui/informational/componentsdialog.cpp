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

  bool IsMultitrackPlugin(const ZXTune::Plugin& plugin)
  {
    assert(!IsPlayerPlugin(plugin));
    return ZXTune::CAP_STOR_MULTITRACK == (plugin.Capabilities() & ZXTune::CAP_STOR_MULTITRACK);
  }

  class ComponentsDialogImpl : public ComponentsDialog
                             , private Ui::ComponentsDialog
  {
  public:
    explicit ComponentsDialogImpl(QWidget* /*parent*/)
    {
      //do not set parent
      setupUi(this);
      FillPluginsTree();
    }

    virtual void Show()
    {
      exec();
    }
  private:
    void AddVersion(const ZXTune::Plugin& plugin, QTreeWidgetItem* root) const
    {
      const String& version = plugin.Version();
      QString str = tr("Version: ");
      str += ToQString(version);
      new QTreeWidgetItem(root, QStringList(str));
    }

    void AddConversionCap(uint_t caps, uint_t mask, QTreeWidgetItem* root, const char* notation) const
    {
      if (mask == (caps & mask))
      {
        const QString& title = tr(notation);
        new QTreeWidgetItem(root, QStringList(title));
      }
    }

    void FillConversionCapabilities(uint_t caps, QTreeWidgetItem* root) const
    {
      AddConversionCap(caps, ZXTune::CAP_CONV_RAW, root, "Raw ripping");
      AddConversionCap(caps, ZXTune::CAP_CONV_OUT, root, "Streamed .out format");
      AddConversionCap(caps, ZXTune::CAP_CONV_PSG, root, "Streamed .psg format");
      AddConversionCap(caps, ZXTune::CAP_CONV_YM, root, "Streamed .ym format");
      AddConversionCap(caps, ZXTune::CAP_CONV_ZX50, root, "Streamed .zx50 format");
      AddConversionCap(caps, ZXTune::CAP_CONV_TXT, root, "Vortex .txt format");
    }

    void AddPlayerPluginItem(QTreeWidgetItem* root, const ZXTune::Plugin& plugin) const
    {
      assert(IsPlayerPlugin(plugin));
      const String& description = plugin.Description();
      const String& id = plugin.Id();
      const QString& conversionTitle = tr("Conversion targets");

      //root
      QTreeWidgetItem* const pluginItem = new QTreeWidgetItem(root, QStringList(ToQString(description)));
      String iconPath = Text::TYPEICONS_RESOURCE_PREFIX;
      iconPath += id;
      pluginItem->setIcon(0, QIcon(ToQString(iconPath)));
      //version
      AddVersion(plugin, pluginItem);
      //conversion
      QTreeWidgetItem* const conversionItem = new QTreeWidgetItem(pluginItem, QStringList(conversionTitle));
      FillConversionCapabilities(plugin.Capabilities() & ZXTune::CAP_CONVERSION_MASK, conversionItem);
    }

    void AddContainerPluginItem(QTreeWidgetItem* root, const ZXTune::Plugin& plugin)
    {
      assert(!IsPlayerPlugin(plugin));
      const String& description = plugin.Description();

      QTreeWidgetItem* const pluginItem = new QTreeWidgetItem(root, QStringList(ToQString(description)));
      AddVersion(plugin, pluginItem);
    }

    void FillPluginsTree()
    {
      const QString& playersTitle = tr("Player plugins");
      const QString& aymsTitle = tr("AY/YM");
      const QString& dacsTitle = tr("DAC");
      const QString& containersTitle = tr("Container plugins");
      const QString& multitracksTitle = tr("Multitrack");
      const QString& implicitsTitle = tr("Implicit");

      QTreeWidgetItem* const players = new QTreeWidgetItem(QStringList(playersTitle));
      QTreeWidgetItem* const containers = new QTreeWidgetItem(QStringList(containersTitle));

      //1st level
      pluginsTree->addTopLevelItem(players);
      pluginsTree->addTopLevelItem(containers);
      //2nd level
      QTreeWidgetItem* const ayms = new QTreeWidgetItem(players, QStringList(aymsTitle));
      QTreeWidgetItem* const dacs = new QTreeWidgetItem(players, QStringList(dacsTitle));
      QTreeWidgetItem* const multitracks = new QTreeWidgetItem(containers, QStringList(multitracksTitle));
      QTreeWidgetItem* const implicits = new QTreeWidgetItem(containers, QStringList(implicitsTitle));

      for (ZXTune::Plugin::Iterator::Ptr plugins = ZXTune::EnumeratePlugins(); plugins->IsValid(); plugins->Next())
      {
        const ZXTune::Plugin::Ptr plugin = plugins->Get();

        if (IsPlayerPlugin(*plugin))
        {
          if (IsAYMPlugin(*plugin))
          {
            AddPlayerPluginItem(ayms, *plugin);
          }
          else
          {
            AddPlayerPluginItem(dacs, *plugin);
          }
        }
        else
        {
          if (IsMultitrackPlugin(*plugin))
          {
            AddContainerPluginItem(multitracks, *plugin);
          }
          else
          {
            AddContainerPluginItem(implicits, *plugin);
          }
        }
      }
    }
  };
}

ComponentsDialog* ComponentsDialog::Create(QWidget* parent)
{
  return new ComponentsDialogImpl(parent);
}
