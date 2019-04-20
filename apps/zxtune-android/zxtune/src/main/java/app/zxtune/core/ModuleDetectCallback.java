package app.zxtune.core;

import android.support.annotation.NonNull;

public interface ModuleDetectCallback {
  void onModule(@NonNull String subpath, @NonNull Module obj);
}
