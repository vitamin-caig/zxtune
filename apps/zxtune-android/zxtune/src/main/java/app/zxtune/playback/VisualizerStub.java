/**
 *
 * @file
 *
 * @brief Stub singleton implementation of Visualizer
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

public class VisualizerStub implements Visualizer {

  @Override
  public int getSpectrum(int[] bands, int[] levels) {
    return 0;
  }

  public static Visualizer instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Visualizer INSTANCE = new VisualizerStub();
  }  
}
