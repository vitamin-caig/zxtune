package app.zxtune.auth;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.UUID;

public class Auth {

  public static String getUserIdentifier(Context ctx) {
    final SharedPreferences prefs = ctx.getSharedPreferences("auth", Context.MODE_PRIVATE);
    final String key = "userid_v1";
    if (prefs.contains(key)) {
      return prefs.getString(key, key);
    } else {
      final String val = generateAnonymousUserIdentifier();
      prefs.edit().putString(key, val).apply();
      return val;
    }
  }

  private static String generateAnonymousUserIdentifier() {
    return "anon" + UUID.randomUUID().toString().substring(0, 8);
  }
}
