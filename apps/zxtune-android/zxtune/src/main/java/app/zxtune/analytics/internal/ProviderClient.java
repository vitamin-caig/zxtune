package app.zxtune.analytics.internal;

import android.content.ContentResolver;
import android.content.Context;

final class ProviderClient implements UrlsSink {

  private final ContentResolver resolver;

  ProviderClient(Context ctx) {
    resolver = ctx.getContentResolver();
    /*
    final Thread thread = new Thread("IASender") {
      @Override
      public void run() {
        try {
          doSending(queue, resolver);
        } catch (InterruptedException e) {
          Log.w(TAG, e, "Stopped IASender thread");
        } finally {
          active.set(false);
        }
      }
    };
    thread.setDaemon(true);
    thread.start();
    */
  }

  @Override
  public void push(String msg) {
    resolver.call(Provider.URI, Provider.METHOD_PUSH, msg, null);
  }

  /*
  private static void doSending(LinkedBlockingQueue<String> queue, ContentResolver resolver) throws InterruptedException {
    final int BATCH_SIZE = 100;
    final ArrayList<String> elements = new ArrayList<>(BATCH_SIZE);
    for (;;) {
      elements.add(queue.take());
      queue.drainTo(elements, BATCH_SIZE - 1);

      final Bundle result = resolver.call(PROVIDER, Provider.METHOD_ADD, null, toBundle(elements));
      elements.clear();
    }
  }

  private static Bundle toBundle(ArrayList<String> elements) {
    final Bundle result = new Bundle();
    result.putStringArrayList("messages", elements);
    return result;
  }
  */
}
