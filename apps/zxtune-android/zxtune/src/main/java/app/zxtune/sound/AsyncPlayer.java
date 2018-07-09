/**
 *
 * @file
 *
 * @brief Asynchronous implementation of Player
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

import android.support.annotation.NonNull;

import app.zxtune.Log;

/**
 * Asynchronous player state machine
 * Synchronized methods protect this from parallel modifications.
 * State's monitor used to synchronize state transitions
 */
public final class AsyncPlayer implements Player {

  private static final String TAG = AsyncPlayer.class.getName();

  private SyncPlayer sync;
  private final PlayerEventsListener events;
  private final Object stateGuard;
  private Player state;
  private Thread playThread;

  private AsyncPlayer(SamplesSource source, SamplesTarget target, PlayerEventsListener events) throws Exception {
    this.sync = new SyncPlayer(source, target, new EventsSynchronized());
    this.events = events;
    this.stateGuard = new Object();
    this.state = new StoppedPlayer();
  }

  public static Player create(SamplesSource source, SamplesTarget target, PlayerEventsListener events) throws Exception {
    return new AsyncPlayer(source, target, events);
  }

  @Override
  public void startPlayback() {
    Log.d(TAG, "Play");
    synchronized (stateGuard) {
      state.startPlayback();
    }
  }

  @Override
  public void stopPlayback() {
    Log.d(TAG, "Stop");
    synchronized (stateGuard) {
      state.stopPlayback();
    }
  }

  @Override
  public void pausePlayback() {
    Log.d(TAG, "Pause");
    synchronized (stateGuard) {
      state.pausePlayback();
    }
  }
  
  @Override
  public boolean isStarted() {
    synchronized (stateGuard) {
      return state.isStarted();
    }
  }

  @Override
  public boolean isPaused() {
    synchronized (stateGuard) {
      return state.isPaused();
    }
  }
  
  @Override
  public synchronized void release() {
    sync.release();
    sync = null;
    state = null;
  }
  
  private void setState(Player newState) {
    synchronized (stateGuard) {
      state = newState;
      stateGuard.notify();
    }
  }
  
  private void waitForStateChange() {
    synchronized (stateGuard) {
      while (true) {
        try {
          stateGuard.wait();
          break;
        } catch (InterruptedException e) {
          Log.w(TAG, e, "Interrupted while waiting for state change");
        }
      }
    }
  }

  private final class EventsSynchronized implements PlayerEventsListener {

    @Override
    public void onStart() {
      setState(new StartedPlayer());
      events.onStart();
    }

    @Override
    public void onFinish() {
      events.onFinish();
    }

    @Override
    public void onStop() {
      setState(new StoppedPlayer());
      events.onStop();
    }

    @Override
    public void onPause() {
      setState(new PausedPlayer());
      events.onPause();
    }

    @Override
    public void onError(@NonNull Exception e) {
      events.onError(e);
    }
  }

  private void doStart() {
    playThread = new Thread("PlayerThread") {
      @Override
      public void run() {
        sync.play();
      }
    };
    playThread.start();
    waitForStateChange();
  }

  private void doStop() {
    try {
      sync.stop();
      waitForStateChange();
      playThread.join();
      playThread = null;
    } catch (InterruptedException e) {
      Log.w(TAG, e, "Interrupted while stop");
    }
  }

  private void doPause() {
    sync.pause();
    waitForStateChange();
  }

  private void doResume() {
    sync.resume();
    waitForStateChange();
  }
  
  private final class StoppedPlayer extends StubPlayer {
    
    StoppedPlayer() {
      Log.d(TAG, "Stopped");
    }

    @Override
    public void startPlayback() {
      doStart();
    }
  }

  private final class StartedPlayer extends StubPlayer {
    
    StartedPlayer() {
      Log.d(TAG, "Started");
    }
    
    @Override
    public void stopPlayback() {
      doStop();
    }

    @Override
    public void pausePlayback() {
      doPause();
    }

    @Override
    public boolean isStarted() {
      return true;
    }
  }

  private final class PausedPlayer extends StubPlayer {

    PausedPlayer() {
      Log.d(TAG, "Paused");
    }

    @Override
    public void startPlayback() {
      doResume();
    }

    @Override
    public void stopPlayback() {
      doStop();
    }

    @Override
    public boolean isPaused() {
      return true;
    }
  }
}
