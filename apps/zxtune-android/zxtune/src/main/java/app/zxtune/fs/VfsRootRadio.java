package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.Callable;

import javax.annotation.CheckForNull;

import app.zxtune.Log;
import app.zxtune.R;

@Icon(R.drawable.ic_browser_vfs_radio)
final class VfsRootRadio extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootRadio.class.getName();

  private final Uri URI = Uri.fromParts("radio", "", "");

  private final Context ctx;
  @CheckForNull
  private ArrayList<VfsDir> children;

  VfsRootRadio(Context ctx) {
    this.ctx = ctx;
  }

  @Nullable
  @Override
  public VfsObject resolve(Uri uri) {
    if (URI.equals(uri)) {
      return this;
    } else {
      return null;
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (VfsDir child : getChildren()) {
      visitor.onDir(child);
    }
  }

  @Override
  public Uri getUri() {
    return URI;
  }

  @Override
  public String getName() {
    return ctx.getString(R.string.vfs_radio_root_name);
  }

  @Override
  public String getDescription() {
    return ctx.getString(R.string.vfs_radio_root_description);
  }

  @Nullable
  @Override
  public VfsObject getParent() {
    return null;
  }

  private ArrayList<VfsDir> getChildren() {
    if (children == null) {
      children = new ArrayList<>();
      maybeAdd(children, EntryModarchive::new);
      maybeAdd(children, EntryVgmrips::new);
    }
    return children;
  }

  private static void maybeAdd(ArrayList<VfsDir> list, Callable<VfsDir> create) {
    try {
      list.add(create.call());
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to add radio source");
    }
  }

  private static class BaseEntry extends StubObject implements VfsDir {

    private final VfsDir delegate;

    BaseEntry(String uri) throws IOException {
      this.delegate = (VfsDir) Vfs.resolve(Uri.parse(uri));
    }

    @Override
    public Uri getUri() {
      return delegate.getUri();
    }

    @Override
    public String getName() {
      return delegate.getName();
    }

    @Override
    public String getDescription() {
      return delegate.getDescription();
    }

    @Override
    public VfsObject getParent() {
      // not required really
      return delegate.getParent();
    }

    @Override
    public Object getExtension(String id) {
      return delegate.getExtension(id);
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      delegate.enumerate(visitor);
    }
  }

  // TODO: get rid of wrappers after browser rework
  @Icon(R.drawable.ic_browser_vfs_modarchive)
  private class EntryModarchive extends BaseEntry {

    EntryModarchive() throws IOException {
      super("modarchive:/Random");
    }

    @Override
    public String getDescription() {
      return ctx.getString(R.string.vfs_modarchive_root_name);
    }
  }

  @Icon(R.drawable.ic_browser_vfs_vgmrips)
  private class EntryVgmrips extends BaseEntry {

    EntryVgmrips() throws IOException {
      super("vgmrips:/Random");
    }

    @Override
    public String getDescription() {
      return ctx.getString(R.string.vfs_vgmrips_root_name);
    }
  }
}
