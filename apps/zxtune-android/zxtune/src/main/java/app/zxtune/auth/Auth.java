package app.zxtune.auth;

import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;

import java.util.UUID;

public class Auth {

  public static String getUserIdentifier(Context ctx) {
    final SharedPreferences prefs = ctx.getSharedPreferences("auth", Context.MODE_PRIVATE);
    final String key = "userid_v1";
    final String stored = prefs.getString(key, null);
    if (!TextUtils.isEmpty(stored)) {
      return stored;
    } else {
      final String generated = generateAnonymousUserIdentifier();
      prefs.edit().putString(key, generated).apply();
      return generated;
    }
  }

  private static String generateAnonymousUserIdentifier() {
    return "anon" + UUID.randomUUID().toString().substring(0, 8);
  }
}
