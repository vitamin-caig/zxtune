package app.zxtune.playback.service;

import app.zxtune.core.Player;
import app.zxtune.playback.Visualizer;

class PlaybackVisualizer implements Visualizer {

  private final Player player;

  PlaybackVisualizer(Player player) {
    this.player = player;
  }

  @Override
  public int getSpectrum(int[] bands, int[] levels) throws Exception {
    return player.analyze(bands, levels);
  }
}
