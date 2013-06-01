/**
 * @file
 * @brief Stub player implementation
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

import app.zxtune.TimeStamp;

public class StubPlayer implements Player {
  
  // permit inheritance
  protected StubPlayer() {}

  @Override
  public void startPlayback() {}

  @Override
  public void stopPlayback() {}
  
  @Override
  public void setPosition(TimeStamp pos) {}
  
  @Override
  public boolean isStarted() {
    return false;
  }
  
  @Override
  public TimeStamp getPosition() {
    return TimeStamp.EMPTY;
  }

  @Override
  public void release() {}

  public static Player instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Player INSTANCE = new StubPlayer();
  }
}
