package app.zxtune.fs;

public interface ProgressCallback {
  void onProgressUpdate(int done, int total);
}
