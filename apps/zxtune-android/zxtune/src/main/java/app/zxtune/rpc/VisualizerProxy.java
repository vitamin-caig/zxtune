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
    public int getSpectrum(byte[] levels) {
      try {
        return delegate.getSpectrum(levels);
      } catch (DeadObjectException e) {
        throw new IllegalStateException(e);
      } catch (RemoteException e) {
        Log.w(TAG, e, "getSpectrum()");
        return 0;
      }
    }
  }

  private static class ServerStub extends IVisualizer.Stub {

    private final Visualizer delegate;

    ServerStub(Visualizer delegate) {
      this.delegate = delegate;
    }

    @Override
    public int getSpectrum(byte[] levels) {
      try {
        return delegate.getSpectrum(levels);
      } catch (Exception e) {
        Log.w(TAG, e, "getSpectrum()");
      }
      return 0;
    }

  }

}
