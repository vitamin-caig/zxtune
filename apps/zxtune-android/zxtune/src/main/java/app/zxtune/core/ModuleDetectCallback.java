package app.zxtune.core;

import androidx.annotation.NonNull;

public interface ModuleDetectCallback {
  void onModule(@NonNull String subpath, @NonNull Module obj);
  void onProgress(int progress);
}
