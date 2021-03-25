package app.zxtune.preferences;

import android.text.TextUtils;

import app.zxtune.Log;
import app.zxtune.core.Properties;
import app.zxtune.core.PropertiesModifier;

/**
 * Adapts generic properties transport to PropertiesModifier
 */
class RawPropertiesAdapter {

  private static final String TAG = RawPropertiesAdapter.class.getName();

  private final PropertiesModifier target;

  RawPropertiesAdapter(PropertiesModifier target) {
    this.target = target;
  }

  final void setProperty(String name, Object value) {
    if (!name.startsWith(Properties.PREFIX)) {
      return;
    }
    try {
      if (value instanceof String) {
        setProperty(name, (String) value);
      } else if (value instanceof Long) {
        target.setProperty(name, (Long) value);
      } else if (value instanceof Integer) {
        target.setProperty(name, ((Integer) value).longValue());
      } else if (value instanceof Boolean) {
        target.setProperty(name, (Boolean) value ? 1 : 0);
      } else {
        throw new Exception("Unknown type of property: " + value.getClass().getName());
      }
    } catch (Exception e) {
      Log.w(TAG, e, "setProperty");
    }
  }

  private void setProperty(String name, String value) {
    if (isNumber(value)) {
      target.setProperty(name, Long.parseLong(value));
    } else {
      target.setProperty(name, value);
    }
  }

  private static boolean isNumber(CharSequence seq) {
    final int size = seq.length();
    if (size == 0) {
      return false;
    }
    final char first = seq.charAt(0);
    if (first == '-' || first == '+') {
      return size > 1 && TextUtils.isDigitsOnly(seq.subSequence(1, size));
    } else if (TextUtils.isDigitsOnly(seq)) {
      return true;
    } else {
      return false;
    }
  }
}
