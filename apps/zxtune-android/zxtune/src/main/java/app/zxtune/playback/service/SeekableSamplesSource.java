package app.zxtune.playback.service;

import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;
import app.zxtune.playback.SeekControl;
import app.zxtune.sound.SamplesSource;

class SeekableSamplesSource implements SamplesSource, SeekControl {

  private ZXTune.Player player;
  private final TimeStamp totalDuration;
  private final TimeStamp frameDuration;
  private volatile TimeStamp seekRequest;

  SeekableSamplesSource(ZXTune.Player player, TimeStamp totalDuration) throws Exception {
    this.player = player;
    this.totalDuration = totalDuration;
    final long frameDurationUs = player.getProperty(ZXTune.Properties.Sound.FRAMEDURATION, ZXTune.Properties.Sound.FRAMEDURATION_DEFAULT);
    this.frameDuration = TimeStamp.createFrom(frameDurationUs, TimeUnit.MICROSECONDS);
    player.setPosition(0);
  }

  @Override
  public void initialize(int sampleRate) throws Exception {
    player.setProperty(ZXTune.Properties.Sound.FREQUENCY, sampleRate);
  }

  @Override
  public boolean getSamples(short[] buf) throws Exception {
    if (seekRequest != null) {
      final int frame = (int) seekRequest.divides(frameDuration);
      player.setPosition(frame);
      seekRequest = null;
    }
    return player.render(buf);
  }

  @Override
  public void release() {
    player.release();
    player = null;
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
