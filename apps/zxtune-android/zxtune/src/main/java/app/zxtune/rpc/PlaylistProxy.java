package app.zxtune.rpc;

import android.os.IBinder;
import android.os.RemoteException;
import app.zxtune.Log;
import app.zxtune.playback.PlaylistControl;

public final class PlaylistProxy {

  private static final String TAG = VisualizerProxy.class.getName();

  public static PlaylistControl getClient(IBinder binder) {
    return new ClientStub(binder);
  }

  public static IBinder getServer(PlaylistControl iface) {
    return new ServerStub(iface);
  }

  private static class ClientStub implements PlaylistControl {

    private final IPlaylist delegate;

    ClientStub(IBinder binder) {
      this.delegate = IPlaylist.Stub.asInterface(binder);
    }

    @Override
    public void delete(long[] ids) {
      try {
        delegate.delete(ids);
      } catch (RemoteException e) {
        Log.w(TAG, e, "delete()");
      }
    }

    @Override
    public void deleteAll() {
      try {
        delegate.deleteAll();
      } catch (RemoteException e) {
        Log.w(TAG, e, "deleteAll()");
      }
    }

    @Override
    public void move(long id, int delta) {
      try {
        delegate.move(id, delta);
      } catch (RemoteException e) {
        Log.w(TAG, e, "move()");
      }
    }

    @Override
    public void sort(SortBy field, SortOrder order) {
      try {
        delegate.sort(field.name(), order.name());
      } catch (RemoteException e) {
        Log.w(TAG, e, "sort()");
      }
    }
  }

  private static class ServerStub extends IPlaylist.Stub {

    private final PlaylistControl delegate;

    ServerStub(PlaylistControl delegate) {
      this.delegate = delegate;
    }
    @Override
    public void delete(long[] ids) {
      delegate.delete(ids);
    }

    @Override
    public void deleteAll() {
      delegate.deleteAll();
    }

    @Override
    public void move(long id, int delta) {
      delegate.move(id, delta);
    }

    @Override
    public void sort(String field, String order) {
      delegate.sort(PlaylistControl.SortBy.valueOf(field), PlaylistControl.SortOrder.valueOf(order));
    }
  }

}
