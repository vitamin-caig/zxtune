/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune;

/**
 * 
 */
public final class ReleaseableStub implements Releaseable {

  private ReleaseableStub() {
  }

  @Override
  public void release() {
  }

  public static Releaseable instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Releaseable INSTANCE = new ReleaseableStub();
  }  
}
