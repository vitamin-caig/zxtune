package app.zxtune.core.jni;

@SuppressWarnings({"unused"})
public class JniRuntimeException extends RuntimeException {
  JniRuntimeException(String msg) {
    super(msg);
  }

  JniRuntimeException(String msg, Throwable e) {
    super(msg, e);
  }
}
