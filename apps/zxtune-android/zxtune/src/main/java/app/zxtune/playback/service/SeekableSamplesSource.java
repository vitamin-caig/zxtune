package app.zxtune.playback.service;

import android.support.annotation.NonNull;

import java.util.concurrent.TimeUnit;

import app.zxtune.Analytics;
import app.zxtune.TimeStamp;
import app.zxtune.core.Player;
import app.zxtune.core.Properties;
import app.zxtune.playback.SeekControl;
import app.zxtune.sound.SamplesSource;

class SeekableSamplesSource implements SamplesSource, SeekControl {

  private Player player;
  private final TimeStamp totalDuration;
  private final TimeStamp frameDuration;
  private volatile TimeStamp seekRequest;

  SeekableSamplesSource(Player player, TimeStamp totalDuration) throws Exception {
    this.player = player;
    this.totalDuration = totalDuration;
    final long frameDurationUs = player.getProperty(Properties.Sound.FRAMEDURATION, Properties.Sound.FRAMEDURATION_DEFAULT);
    this.frameDuration = TimeStamp.createFrom(frameDurationUs, TimeUnit.MICROSECONDS);
    player.setPosition(0);
  }

  @Override
  public void initialize(int sampleRate) throws Exception {
    player.setProperty(Properties.Sound.FREQUENCY, sampleRate);
  }

  @Override
  public boolean getSamples(@NonNull short[] buf) throws Exception {
    if (seekRequest != null) {
      final int frame = (int) seekRequest.divides(frameDuration);
      player.setPosition(frame);
      seekRequest = null;
    }
    if (player.render(buf)) {
      return true;
    } else {
      player.setPosition(0);
      return false;
    }
  }

  @Override
  public void release() {
    Analytics.sendPerformanceEvent(player);
    player.release();
  }

  @Override
  public TimeStamp getDuration() {
    return totalDuration;
  }

  @Override
  public TimeStamp getPosition() throws Exception {
    TimeStamp res = seekRequest;
    if (res == null) {
      final int frame = player.getPosition();
      res = frameDuration.multiplies(frame);
    }
    return res;
  }

  @Override
  public void setPosition(TimeStamp pos) {
    seekRequest = pos;
  }
}
