package app.zxtune.core.jni;

//should be single entrypoint to lock on
final class JniLibrary {

  static {
    System.loadLibrary("zxtune");
  }

  static void load() {
    // noop
  }
}
