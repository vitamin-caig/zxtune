package app.zxtune.preferences;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import androidx.annotation.Nullable;

import app.zxtune.BroadcastReceiverConnection;
import app.zxtune.PreferencesActivity;
import app.zxtune.Releaseable;
import app.zxtune.core.PropertiesModifier;

public class ChangedSettingsReceiver extends BroadcastReceiver {

  private static final String TAG = ChangedSettingsReceiver.class.getName();

  private final RawPropertiesAdapter target;

  private ChangedSettingsReceiver(PropertiesModifier options) {
    this.target = new RawPropertiesAdapter(options);
  }

  public static Releaseable subscribe(Context ctx, PropertiesModifier options) {
    return new BroadcastReceiverConnection(ctx,
        new ChangedSettingsReceiver(options),
        new IntentFilter(PreferencesActivity.ACTION_PREFERENCE_CHANGED)
    );
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    @Nullable final String key = intent.getStringExtra(PreferencesActivity.EXTRA_PREFERENCE_NAME);
    @Nullable final Object value = intent.getExtras().get(PreferencesActivity.EXTRA_PREFERENCE_VALUE);
    if (key != null && value != null) {
      target.setProperty(key, value);
    }
  }
}
