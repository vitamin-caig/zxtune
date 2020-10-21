package app.zxtune.sound;

import app.zxtune.TimeStamp;

final public class StubSamplesSource implements SamplesSource {

  private StubSamplesSource() {}

  @Override
  public boolean getSamples(short[] buf) {
    return false;
  }

  @Override
  public void setPosition(TimeStamp pos) {}

  @Override
  public TimeStamp getPosition() {
    return TimeStamp.EMPTY;
  }


  public static SamplesSource instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    static final StubSamplesSource INSTANCE = new StubSamplesSource();
  }

}
