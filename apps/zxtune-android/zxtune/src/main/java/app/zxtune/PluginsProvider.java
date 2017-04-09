/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.support.annotation.NonNull;

public final class PluginsProvider extends ContentProvider {
  
  public enum Columns {
    Type,
    Description
  }
  
  //Identifiers describe relative order in UI view, so do not use raw string identifiers
  public enum Types {
    PlayerAymTs(R.string.plugin_player_aym_ts),
    PlayerDac(R.string.plugin_player_dac),
    PlayerFmTfm(R.string.plugin_player_fm_tfm),
    PlayerSaa(R.string.plugin_player_saa),
    PlayerSid(R.string.plugin_player_sid),
    PlayerSpc(R.string.plugin_player_spc),
    PlayerRp2a0x(R.string.plugin_player_rp2a0x),
    PlayerLr35902(R.string.plugin_player_lr35902),
    PlayerCo12294(R.string.plugin_player_co12294),
    PlayerHuc6270(R.string.plugin_player_huc6270),
    PlayerMultidevice(R.string.plugin_player_multidevice),
    
    ContainerMultitrack(R.string.plugin_multitrack),
    ContainerArchive(R.string.plugin_archive),
    ContainerDecompiler(R.string.plugin_decompiler),
    
    Unknown(R.string.plugin_unknown);
    
    private final int nameId;
    
    private Types(int id) {
      this.nameId = id;
    }
    
    public final int nameId() {
      return nameId;
    }
  }
  
  private static final String AUTHORITY = "app.zxtune.plugins";
  private static final String MIME = "vnd.android.cursor.dir/vnd." + AUTHORITY;
  
  @Override
  public boolean onCreate() {
    return true;
  }
  
  @Override
  public int delete(@NonNull Uri arg0, String arg1, String[] arg2) {
    return 0;
  }

  @Override
  public String getType(@NonNull Uri uri) {
    return MIME;
  }

  @Override
  public Uri insert(@NonNull Uri uri, ContentValues values) {
    return null;
  }

  @Override
  public int update(@NonNull Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }
  
  @Override
  public Cursor query(@NonNull Uri uri, String[] projection, String selection, String[] selectionArgs,
      String sortOrder) {
    final String[] columns = {Columns.Type.name(), Columns.Description.name()}; 
    final MatrixCursor res = new MatrixCursor(columns);
    ZXTune.Plugins.enumerate(new ZXTune.Plugins.Visitor() {
      @Override
      public void onPlayerPlugin(int devices, String id, String description) {
        final int type = getPlayerPluginType(devices).ordinal();
        final String descr = String.format("[%s] %s", id, description);
        final Object[] values = {type, descr};
        res.addRow(values);
      }

      @Override
      public void onContainerPlugin(int containerType, String id, String description) {
        final int type = getContainerPluginType(containerType).ordinal();
        final Object[] values = {type, description};
        res.addRow(values);
      }
    });
    return res;
  }

  private static Types getPlayerPluginType(int devices) {
    //TODO: extract separate strings
    if (0 != (devices & (ZXTune.Plugins.DeviceType.AY38910 | ZXTune.Plugins.DeviceType.TURBOSOUND))) {
      return Types.PlayerAymTs;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.DAC)) {
      return Types.PlayerDac;
    } else if (0 != (devices & (ZXTune.Plugins.DeviceType.YM2203 | ZXTune.Plugins.DeviceType.TURBOFM))) {
      return Types.PlayerFmTfm;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.SAA1099)) {
      return Types.PlayerSaa;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.MOS6581)) {
      return Types.PlayerSid;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.SPC700)) {
      return Types.PlayerSpc;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.MULTIDEVICE)) {
      return Types.PlayerMultidevice;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.RP2A0X)) {
      return Types.PlayerRp2a0x;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.LR35902)) {
      return Types.PlayerLr35902;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.CO12294)) {
      return Types.PlayerCo12294;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.HUC6270)) {
      return Types.PlayerHuc6270;
    } else {
      return Types.Unknown;
    }
  }
  
  private static Types getContainerPluginType(int type) {
    switch (type) {
      case ZXTune.Plugins.ContainerType.ARCHIVE:
        return Types.ContainerArchive;
      case ZXTune.Plugins.ContainerType.DECOMPILER:
        return Types.ContainerDecompiler;
      case ZXTune.Plugins.ContainerType.MULTITRACK:
        return Types.ContainerMultitrack;
      default:
        return Types.Unknown;
    }
  }
  
  public static Uri getUri() {
    return new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY).build();
  }
}
