package app.zxtune.playback.service;

import app.zxtune.TimeStamp;
import app.zxtune.core.Player;
import app.zxtune.core.Properties;
import app.zxtune.sound.SamplesSource;

import java.util.concurrent.TimeUnit;

class SeekableSamplesSource implements SamplesSource {

  private final Player player;

  SeekableSamplesSource(Player player) {
    this.player = player;
    player.setPosition(TimeStamp.EMPTY);
  }

  @Override
  public boolean getSamples(short[] buf) {
    if (player.render(buf)) {
      return true;
    } else {
      player.setPosition(TimeStamp.EMPTY);
      return false;
    }
  }

  @Override
  public TimeStamp getPosition() {
    return player.getPosition();
  }

  @Override
  public void setPosition(TimeStamp pos) {
    player.setPosition(pos);
  }
}
