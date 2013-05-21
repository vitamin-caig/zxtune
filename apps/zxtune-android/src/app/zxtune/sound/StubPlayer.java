/**
 * @file
 * @brief Stub player implementation
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

public class StubPlayer implements Player {

  @Override
  public void play() {}

  @Override
  public void stop() {}

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
