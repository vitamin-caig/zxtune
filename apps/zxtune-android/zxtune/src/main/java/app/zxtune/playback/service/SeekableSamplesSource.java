package app.zxtune.playback.service;

import androidx.annotation.NonNull;
import app.zxtune.TimeStamp;
import app.zxtune.core.Player;
import app.zxtune.core.Properties;
import app.zxtune.sound.SamplesSource;

import java.util.concurrent.TimeUnit;

class SeekableSamplesSource implements SamplesSource {

  private Player player;
  private final TimeStamp frameDuration;

  SeekableSamplesSource(Player player) {
    this.player = player;
    final long frameDurationUs = player.getProperty(Properties.Sound.FRAMEDURATION, Properties.Sound.FRAMEDURATION_DEFAULT);
    this.frameDuration = TimeStamp.createFrom(frameDurationUs, TimeUnit.MICROSECONDS);
    player.setPosition(0);
  }

  @Override
  public void initialize(int sampleRate) {
    player.setProperty(Properties.Sound.FREQUENCY, sampleRate);
  }

  @Override
  public boolean getSamples(@NonNull short[] buf) {
    if (player.render(buf)) {
      return true;
    } else {
      player.setPosition(0);
      return false;
    }
  }

  @Override
  public TimeStamp getPosition() {
    return frameDuration.multiplies(player.getPosition());
  }

  @Override
  public void setPosition(TimeStamp pos) {
    player.setPosition(toFrame(pos));
  }

  private int toFrame(TimeStamp pos) {
    return (int) pos.divides(frameDuration);
  }
}
