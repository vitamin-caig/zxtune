package app.zxtune.utils;

import android.os.Handler;
import android.os.HandlerThread;

public class AsyncWorker {

  private final HandlerThread thread;
  private final Handler handler;

  public AsyncWorker(String id) {
    this.thread = new HandlerThread(id);
    thread.start();
    this.handler = new Handler(thread.getLooper());
  }

  public AsyncWorker execute(Runnable task) {
    handler.post(task);
    return this;
  }
}
