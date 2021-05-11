package app.zxtune.preferences;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Map;

import javax.annotation.CheckForNull;

import app.zxtune.Log;
import app.zxtune.MainApplication;
import app.zxtune.Preferences;

public class Provider extends android.content.ContentProvider {

  private static final String TAG = Provider.class.getName();

  final static Uri URI = new Uri.Builder()
      .scheme(ContentResolver.SCHEME_CONTENT)
      .authority("app.zxtune.preferences")
      .build();

  final static String METHOD_GET = "get";
  final static String METHOD_LIST = "list";
  final static String METHOD_PUT = "put";

  @CheckForNull
  private SharedPreferences prefs;

  @Override
  public boolean onCreate() {
    final Context ctx = getContext();
    if (ctx != null) {
      MainApplication.initialize(ctx.getApplicationContext());
      prefs = Preferences.getDefaultSharedPreferences(ctx);
      return true;
    } else {
      return false;
    }
  }

  @Nullable
  @Override
  public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
    return null;
  }

  @Nullable
  @Override
  public String getType(@NonNull Uri uri) {
    return null;
  }

  @Nullable
  @Override
  public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
    return null;
  }

  @Override
  public synchronized int delete(@NonNull Uri uri, @Nullable String selection,
                                 @Nullable String[] selectionArgs) {
    if (selectionArgs != null) {
      delete(selectionArgs);
    }
    return 0;
  }

  @Override
  public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
    return 0;
  }

  @Override
  @Nullable
  public synchronized Bundle call(String method, @Nullable String arg, @Nullable Bundle extras) {
    switch (method) {
      case METHOD_GET:
        return get(arg);
      case METHOD_LIST:
        return list(arg);
      case METHOD_PUT:
        put(extras);
        break;
      default:
        break;
    }
    return null;
  }

  @Nullable
  private Bundle get(@Nullable String key) {
    if (key == null) {
      return null;
    }
    final Map<String, ?> all = prefs.getAll();
    final Object res = all.get(key);
    if (res == null) {
      return null;
    }
    final Bundle result = new Bundle();
    copyPref(key, res, result);
    return result;
  }

  private Bundle list(@Nullable String prefix) {
    final Bundle result = new Bundle();
    for (Map.Entry<String, ?> entry : prefs.getAll().entrySet()) {
      final String key = entry.getKey();
      if (prefix == null || key.startsWith(prefix)) {
        copyPref(key, entry.getValue(), result);
      }
    }
    return result;
  }

  private static void copyPref(String key, Object val, Bundle result) {
    if (val instanceof Long) {
      result.putLong(key, (Long) val);
    } else if (val instanceof Integer) {
      result.putInt(key, (Integer) val);
    } else if (val instanceof String) {
      result.putString(key, (String) val);
    } else if (val instanceof Boolean) {
      result.putBoolean(key, (Boolean) val);
    } else {
      throw new IllegalArgumentException("Invalid type for " + key + ": " + val.getClass().getName());
    }
  }

  private void put(Bundle data) {
    final SharedPreferences.Editor editor = prefs.edit();
    for (String key : data.keySet()) {
      final Object obj = data.get(key);
      setPref(key, obj, editor);
    }
    // no disk IO on the main thread
    editor.apply();
  }

  private void setPref(String key, Object val, SharedPreferences.Editor editor) {
    Log.d(TAG, "%s=%s", key, val);
    if (val instanceof Long) {
      editor.putLong(key, (Long) val);
    } else if (val instanceof Integer) {
      editor.putInt(key, (Integer) val);
    } else if (val instanceof String) {
      editor.putString(key, (String) val);
    } else if (val instanceof Boolean) {
      editor.putBoolean(key, (Boolean) val);
    } else {
      throw new IllegalArgumentException("Invalid type for " + key + ": " + val.getClass().getName());
    }
  }

  private void delete(String[] keys) {
    final SharedPreferences.Editor editor = prefs.edit();
    for (String k : keys) {
      editor.remove(k);
    }
    // no disk IO on the main thread
    editor.apply();
  }
}
