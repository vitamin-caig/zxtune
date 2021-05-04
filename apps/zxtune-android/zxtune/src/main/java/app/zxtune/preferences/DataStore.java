package app.zxtune.preferences;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.preference.PreferenceDataStore;

import java.util.Set;

import javax.annotation.CheckForNull;

public class DataStore extends PreferenceDataStore {

  private final ProviderClient client;
  @CheckForNull
  private Bundle cache;

  public DataStore(Context ctx) {
    this(new ProviderClient(ctx));
  }

  @VisibleForTesting
  DataStore(ProviderClient client) {
    this.client = client;
  }

  @Override
  public void putBoolean(String key, boolean value) {
    client.set(key, value);
    if (cache != null) {
      cache.putBoolean(key, value);
    }
  }

  @Override
  public void putInt(String key, int value) {
    client.set(key, value);
    if (cache != null) {
      cache.putInt(key, value);
    }
  }

  @Override
  public void putString(String key, @Nullable String value) {
    client.set(key, value);
    if (cache != null) {
      cache.putString(key, value);
    }
  }

  @Override
  public void putLong(String key, long value) {
    client.set(key, value);
    if (cache != null) {
      cache.putLong(key, value);
    }
  }

  public void putBatch(Bundle batch) {
    client.set(batch);
    if (cache != null) {
      cache.putAll(batch);
    }
  }

  @Nullable
  public String getString(String key, @Nullable String defValue) {
    return getCache().getString(key, defValue);
  }

  @Override
  public int getInt(String key, int defValue) {
    return getCache().getInt(key, defValue);
  }

  @Override
  public long getLong(String key, long defValue) {
    return getCache().getLong(key, defValue);
  }

  @Override
  public boolean getBoolean(String key, boolean defValue) {
    return getCache().getBoolean(key, defValue);
  }

  @Override
  @Nullable
  public Set<String> getStringSet(String key, @Nullable Set<String> defValues) {
    throw new UnsupportedOperationException("Not implemented on this data store");
  }

  @Override
  public float getFloat(String key, float defValue) {
    throw new UnsupportedOperationException("Not implemented on this data store");
  }

  private Bundle getCache() {
    if (cache == null) {
      cache = client.getAll();
    }
    return cache;
  }
}
