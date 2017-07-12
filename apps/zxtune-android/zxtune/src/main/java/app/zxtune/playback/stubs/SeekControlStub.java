/**
 *
 * @file
 *
 * @brief Stub singleton implementation of SeekControl
 *
 * @author vitamin.caig@gmail.com
 *
 */


package app.zxtune.playback.stubs;

import app.zxtune.TimeStamp;
import app.zxtune.playback.SeekControl;

public final class SeekControlStub implements SeekControl {

  private SeekControlStub() {
  }
  
  @Override
  public TimeStamp getDuration() {
    return TimeStamp.EMPTY;
  }

  @Override
  public TimeStamp getPosition() {
    return TimeStamp.EMPTY;
  }

  @Override
  public void setPosition(TimeStamp position) {
  }
  
  public static SeekControl instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final SeekControl INSTANCE = new SeekControlStub();
  }  
}
