package app.zxtune.fs.api;

import android.net.Uri;

public class Cdn {
  public static Uri asma(String path) {
    return getRoot().path("browse/asma/" + path).build();
  }

  public static Uri hvsc(String path) {
    return getRoot().path("browse/hvsc/" + path).build();
  }

  public static Uri joshw(String catalogue, String path) {
    return getRoot().path("browse/joshw/" + catalogue + "/" + path).build();
  }

  public static Uri amp(int id) {
    return getRoot().path("download/amp/ids/" + id).build();
  }

  public static Uri modland(String id) {
    // really url-encoded paths
    return getRoot().encodedPath("download/modland" + id).build();
  }

  public static Uri modarchive(int id) {
    return getRoot().path("download/modarchive/ids/" + id).build();
  }

  private static Uri.Builder getRoot() {
    return new Uri.Builder().scheme("https").authority("storage.zxtune.ru");
  }
}
