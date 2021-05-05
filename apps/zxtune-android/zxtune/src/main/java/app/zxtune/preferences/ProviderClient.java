package app.zxtune.preferences;

import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;

public class ProviderClient {

  private final ContentResolver resolver;

  public ProviderClient(Context ctx) {
    resolver = ctx.getContentResolver();
  }

  public Bundle getAll() {
    return resolver.call(Provider.URI, Provider.METHOD_LIST, null, null);
  }

  public Bundle get(String prefix) {
    return resolver.call(Provider.URI, Provider.METHOD_LIST, prefix, null);
  }

  public void set(String key, String val) {
    final Bundle values = new Bundle();
    values.putString(key, val);
    set(values);
  }

  public void set(String key, int val) {
    final Bundle values = new Bundle();
    values.putInt(key, val);
    set(values);
  }

  public void set(String key, long val) {
    final Bundle values = new Bundle();
    values.putLong(key, val);
    set(values);
  }

  public void set(String key, boolean val) {
    final Bundle values = new Bundle();
    values.putBoolean(key, val);
    set(values);
  }

  public void set(Bundle batch) {
    resolver.call(Provider.URI, Provider.METHOD_PUT, null, batch);
  }
}
