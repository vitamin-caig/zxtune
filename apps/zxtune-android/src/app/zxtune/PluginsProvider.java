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

public final class PluginsProvider extends ContentProvider {
  
  public enum Columns {
    Type,
    Id,
    Description
  }
  
  public final static class Types {
    public final static int PLAYER_AYM_TS = 0;
    public final static int PLAYER_DAC = 1;
    public final static int PLAYER_FM_TFM = 2;
    public final static int PLAYER_SAA = 3;
    public final static int PLAYER_SID = 4;
    public final static int PLAYER_SPC = 5;
    
    //TODO: clarify types in low level
    public final static int DECODER_DECOMPILER = 6;
    
    public final static int MULTITRACK_CONTAINER = 7;

    public final static int UNKNOWN = 8;
  }
  
  private final static String AUTHORITY = "app.zxtune.plugins";
  private final static String MIME = "vnd.android.cursor.dir/vnd." + AUTHORITY;
  
  @Override
  public boolean onCreate() {
    return true;
  }
  
  @Override
  public int delete(Uri arg0, String arg1, String[] arg2) {
    return 0;
  }

  @Override
  public String getType(Uri uri) {
    return MIME;
  }

  @Override
  public Uri insert(Uri uri, ContentValues values) {
    return null;
  }

  @Override
  public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }
  
  @Override
  public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
      String sortOrder) {
    final String[] columns = {Columns.Type.name(), Columns.Id.name(), Columns.Description.name()}; 
    final MatrixCursor res = new MatrixCursor(columns);
    ZXTune.Plugins.enumerate(new ZXTune.Plugins.Visitor() {
      @Override
      public void onPlayerPlugin(int devices, String id, String description) {
        final Object[] values = {getPlayerPluginType(devices), id, description};
        res.addRow(values);
      }

      @Override
      public void onDecoderPlugin(String id, String description) {
        final Object[] values = {Types.DECODER_DECOMPILER, id, description};
        res.addRow(values);
      }

      @Override
      public void onMultitrackPlugin(String id, String description) {
        final Object[] values = {Types.MULTITRACK_CONTAINER, id, description};
        res.addRow(values);
      }
    });
    return res;
  }

  private static int getPlayerPluginType(int devices) {
    //TODO: extract separate strings
    if (0 != (devices & (ZXTune.Plugins.DeviceType.AY38910 | ZXTune.Plugins.DeviceType.TURBOSOUND))) {
      return Types.PLAYER_AYM_TS;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.DAC)) {
      return Types.PLAYER_DAC;
    } else if (0 != (devices & (ZXTune.Plugins.DeviceType.YM2203 | ZXTune.Plugins.DeviceType.TURBOFM))) {
      return Types.PLAYER_FM_TFM;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.SAA1099)) {
      return Types.PLAYER_SAA;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.MOS6581)) {
      return Types.PLAYER_SID;
    } else if (0 != (devices & ZXTune.Plugins.DeviceType.SPC700)) {
      return Types.PLAYER_SPC;
    } else {
      return Types.UNKNOWN;
    }
  }
  
  public static Uri getUri() {
    return new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY).build();
  }
}
