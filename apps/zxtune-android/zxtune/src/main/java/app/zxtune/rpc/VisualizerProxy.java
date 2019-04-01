package app.zxtune.rpc;

import android.os.DeadObjectException;
import android.os.IBinder;
import android.os.RemoteException;
import app.zxtune.Log;
import app.zxtune.playback.Visualizer;

public final class VisualizerProxy {

  private static final String TAG = VisualizerProxy.class.getName();

  public static Visualizer getClient(IBinder binder) {
    return new ClientStub(binder);
  }

  public static IBinder getServer(Visualizer iface) {
    return new ServerStub(iface);
  }

  private static class ClientStub implements Visualizer {

    private final IVisualizer delegate;

    ClientStub(IBinder binder) {
      this.delegate = IVisualizer.Stub.asInterface(binder);
    }

    @Override
    public int getSpectrum(int[] bands, int[] levels) {
      try {
        final int[] packed = delegate.getSpectrum();
        for (int i = 0; i != packed.length; ++i) {
          bands[i] = packed[i] & 0xff;
          levels[i] = packed[i] >> 8;
        }
        return packed.length;
      } catch (DeadObjectException e) {
        throw new IllegalStateException(e);
      } catch (RemoteException e) {
        Log.w(TAG, e, "getSpectrum()");
        return 0;
      }
    }
  }

  private static class ServerStub extends IVisualizer.Stub {

    private static final int MAX_BANDS = 96;
    private final int[] bands = new int[MAX_BANDS];
    private final int[] levels = new int[MAX_BANDS];
    private final Visualizer delegate;

    ServerStub(Visualizer delegate) {
      this.delegate = delegate;
    }

    @Override
    public int[] getSpectrum() {
      try {
        final int chans = delegate.getSpectrum(bands, levels);
        final int[] res = new int[chans];
        for (int idx = 0; idx != chans; ++idx) {
          res[idx] = (levels[idx] << 8) | bands[idx];
        }
        return res;
      } catch (Exception e) {
        Log.w(TAG, e, "getSpectrum()");
      }
      return new int[1];
    }

  }

}
