/**
 * @file
 * @brief Stub singleton implementation of Player
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

public class StubPlayer implements Player {

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
