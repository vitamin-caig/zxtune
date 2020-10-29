/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playlist.xspf;

import android.content.Context;
import android.database.Cursor;
import android.os.Environment;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.playlist.Item;

public class XspfStorage {

  private static final String TAG = XspfStorage.class.getName();
  private static final String EXTENSION = ".xspf";
  private final File root;

  public XspfStorage(Context context) {
    final String path = context.getString(R.string.playlists_storage_path);
    this.root = new File(Environment.getExternalStorageDirectory(), path);
    if (root.mkdirs()) {
      Log.d(TAG, "Created playlists storage dir");
    }
  }

  public final ArrayList<String> enumeratePlaylists() {
    String[] files = root.list();
    if (files == null) {
      files = new String[0];
    }
    final ArrayList<String> result = new ArrayList<>(files.length);
    for (String filename : files) {
      final int extPos = filename.lastIndexOf(EXTENSION);
      if (-1 != extPos) {
        result.add(filename.substring(0, extPos));
      }
    }
    return result;
  }

  @Nullable
  public final String findPlaylistPath(String name) {
    final File res = getFileFor(name);
    return res.isFile() ? res.getAbsolutePath() : null;
  }

  private File getFileFor(String name) {
    return new File(root, name + EXTENSION);
  }

  public final void createPlaylist(String name, Cursor cursor) throws IOException {
    final File file = getFileFor(name);
    final FileOutputStream stream = new FileOutputStream(file);
    final Builder builder = new Builder(stream);
    builder.writePlaylistProperties(name, cursor.getCount());
    while (cursor.moveToNext()) {
      builder.writeTrack(new Item(cursor));
    }
    builder.finish();
  }
}
