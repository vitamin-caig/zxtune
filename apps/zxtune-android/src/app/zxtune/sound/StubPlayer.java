/**
 * @file
 * @brief Stub player implementation
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;


public class StubPlayer implements Player {
  
  // permit inheritance
  protected StubPlayer() {}

  @Override
  public void startPlayback() {}

  @Override
  public void stopPlayback() {}
  
  @Override
  public boolean isStarted() {
    return false;
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
