/**
 * @file
 * @brief Implementation of Player based on AudioTrack functionality
 * @version $Id:$
 * @author
 */
package app.zxtune.sound;

import android.os.Process;
import android.util.Log;

/**
 * Asynchronous player state machine
 * Synchronized methods protect this from parallel modifications.
 * State's monitor used to synchronize state transitions
 */
final public class AsyncPlayer implements Player {

  private static final String TAG = AsyncPlayer.class.getName();

  private Player sync;
  private PlayerEventsListener events;
  private Thread playThread;
  private volatile Player state;

  private AsyncPlayer(SamplesSource source, SamplesTarget target, PlayerEventsListener events) {
    this.sync = new SyncPlayer(source, target, new EventsSynchronized());
    this.events = events;
    this.state = new StoppedPlayer();
  }

  public static Player create(SamplesSource source, PlayerEventsListener events) {
    final SamplesTarget target = SoundOutputSamplesTarget.create();
    return new AsyncPlayer(source, target, events);
  }

  @Override
  public synchronized void play() {
    Log.d(TAG, "Play");
    state.play();
  }

  @Override
  public synchronized void stop() {
    Log.d(TAG, "Stop");
    state.stop();
  }

  @Override
  public synchronized void release() {
    sync.release();
    sync = null;
    state = null;
  }
  
  private void setState(Player newState) {
    final Player oldState = state;
    synchronized (oldState) {
      state = newState;
      oldState.notify();
    }
  }
  
  private void waitForStateChange() {
    try {
      synchronized (state) {
        state.wait();
      }
    } catch (InterruptedException e) {
      Log.d(TAG, "Interrupted while stop: ");
      e.printStackTrace();
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
    public void onError(Error e) {
      events.onError(e);
    }
  }
  
  private final class StoppedPlayer extends StubPlayer {
    
    StoppedPlayer() {
      Log.d(TAG, "Stopped");
    }

    @Override
    public void play() {
      playThread = new PlayThread();
      playThread.start();
      waitForStateChange();
    }
  }

  private final class StartedPlayer extends StubPlayer {
    
    StartedPlayer() {
      Log.d(TAG, "Started");
    }

    @Override
    public void stop() {
      try {
        sync.stop();
        waitForStateChange();
        playThread.join();
        playThread = null;
      } catch (InterruptedException e) {
        Log.d(TAG, "Interrupted while stop: ");
        e.printStackTrace();
      }
    }
  }

  private class PlayThread extends Thread {
    @Override
    public void run() {
      Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
      sync.play();
    }
  };
}
