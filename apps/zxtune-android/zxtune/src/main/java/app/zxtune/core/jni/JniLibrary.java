package app.zxtune.core.jni;

final class JniLibrary {
  static void load() {
    //should be single entrypoint to lock on
    System.loadLibrary("zxtune");
  }
}
