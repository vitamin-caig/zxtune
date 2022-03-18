package app.zxtune.analytics.internal;

import java.io.IOException;
import java.util.ArrayDeque;

import app.zxtune.Log;
import app.zxtune.net.NetworkManager;

class Dispatcher implements UrlsSink {

  private static final String TAG = Dispatcher.class.getName();

  private static final int NETWORK_RETRIES_PERIOD = 1000;

  private final NetworkSink online;
  private final BufferSink offline;
  private UrlsSink current;
  private int networkRetryCountdown;

  Dispatcher() {
    this.online = new NetworkSink();
    this.offline = new BufferSink();
    current = online;
    NetworkManager.getNetworkAvailable().observeForever(this::onNetworkChange);
  }

  @Override
  public void push(String url) {
    try {
      current.push(url);
      trySwitchToNetwork();
    } catch (IOException e) {
      Log.w(TAG, e, "Network error");
      current = offline;
      networkRetryCountdown = NETWORK_RETRIES_PERIOD;
    }
  }

  private void trySwitchToNetwork() throws IOException {
    if (networkRetryCountdown > 0 && 0 == --networkRetryCountdown) {
      offline.flushTo(online);
      current = online;
    }
  }

  private void onNetworkChange(boolean isAvailable) {
    Log.d(TAG, "onNetworkChange: " + isAvailable);
    if (isAvailable) {
      if (current == offline) {
        networkRetryCountdown = 1;
      } else {
        current = online;
      }
    } else {
      current = offline;
      networkRetryCountdown = 0;
    }
  }

  private static class BufferSink implements UrlsSink {

    private static final int MAX_BLOCK_SIZE = 1000;
    private static final int MAX_BLOCKS_COUNT = 100;

    private final ArrayDeque<ArrayDeque<String>> buffers = new ArrayDeque<>(MAX_BLOCKS_COUNT);
    private int size = 0;
    private int lost = 0;
    private int trimSize = 1;

    @Override
    public void push(String url) {
      ArrayDeque<String> buf = buffers.peekLast();
      if (buf == null || buf.size() == MAX_BLOCK_SIZE) {
        buf = new ArrayDeque<>(MAX_BLOCK_SIZE);
        buffers.addLast(buf);
        if (buffers.size() > MAX_BLOCKS_COUNT) {
          trimBuffers();
        }
      }
      buf.addLast(url);
      ++size;
    }

    private void trimBuffers() {
      for (int i = trimSize; i > 0; --i) {
        lost += buffers.removeFirst().size();
      }
      size -= lost;
      if (trimSize * 2 < MAX_BLOCKS_COUNT) {
        trimSize *= 2;
      }
    }

    final void flushTo(UrlsSink sink) throws IOException {
      if (size == 0) {
        return;
      }
      sendDiagnostic(sink);
      while (!buffers.isEmpty()) {
        flush(buffers.peekFirst(), sink);
        buffers.removeFirst();
      }
    }

    private void sendDiagnostic(UrlsSink sink) throws IOException {
      sink.push(getDiagnosticUrl());
      lost = 0;
    }

    private String getDiagnosticUrl() {
      final UrlsBuilder builder = new UrlsBuilder("stat/offline");
      builder.addParam("count", size);
      builder.addParam("lost", lost);
      return builder.getResult();
    }

    private void flush(ArrayDeque<String> buf, UrlsSink sink) throws IOException {
      final String suffix = getResendSuffix();
      while (!buf.isEmpty()) {
        sink.push(buf.peekFirst() + suffix);
        buf.removeFirst();
        --size;
      }
    }

    private String getResendSuffix() {
      return "&rts=" + System.currentTimeMillis() / 1000;
    }
  }
}
