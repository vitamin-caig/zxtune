package app.zxtune.sound;

import android.support.annotation.NonNull;
import app.zxtune.TimeStamp;

final public class StubSamplesSource implements SamplesSource {

  private StubSamplesSource() {}

  @Override
  public void initialize(int sampleRate) {}

  @Override
  public boolean getSamples(@NonNull short[] buf) {
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
    public static final StubSamplesSource INSTANCE = new StubSamplesSource();
  }

}
