/**
 * @file
 * @brief Stub implementation of PlayerEventsListener
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

public class StubPlayerEventsListener implements PlayerEventsListener {

  // permit inheritance
  protected StubPlayerEventsListener() {}
  
  @Override
  public void onStart() {}

  @Override
  public void onFinish() {}

  @Override
  public void onStop() {}
  
  @Override
  public void onError(Error e) {}

  public static PlayerEventsListener instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final PlayerEventsListener INSTANCE = new StubPlayerEventsListener();
  }
}
