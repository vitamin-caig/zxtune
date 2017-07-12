package app.zxtune.playback.service;

import app.zxtune.ZXTune;
import app.zxtune.playback.Visualizer;

class PlaybackVisualizer implements Visualizer {

  private final ZXTune.Player player;

  PlaybackVisualizer(ZXTune.Player player) {
    this.player = player;
  }

  @Override
  public int getSpectrum(int[] bands, int[] levels) throws Exception {
    return player.analyze(bands, levels);
  }
}
