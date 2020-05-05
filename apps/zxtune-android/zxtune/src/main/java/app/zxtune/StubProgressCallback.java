package app.zxtune;

import app.zxtune.fs.ProgressCallback;

public final class StubProgressCallback implements ProgressCallback {

  private StubProgressCallback() {}

  @Override
  public void onProgressUpdate(int done, int total) {}

  public static ProgressCallback instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final ProgressCallback INSTANCE = new StubProgressCallback();
  }
}
