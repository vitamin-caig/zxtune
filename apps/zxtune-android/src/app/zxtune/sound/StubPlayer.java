/**
 * @file
 * @brief Stub player implementation
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;

public class StubPlayer implements Player {
  
  private final static TimeStamp position = TimeStamp.createFrom(0, TimeUnit.MILLISECONDS); 

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
    return position;
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
