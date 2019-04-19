package app.zxtune.core;

public class ResolvingException extends Exception {
  private static final long serialVersionUID = 1L;

  ResolvingException(String msg) {
    super(msg);
  }

  ResolvingException(String msg, Throwable e) {
    super(msg, e);
  }
}
