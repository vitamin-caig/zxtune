package app.zxtune.ui.browser;

import android.app.Application;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.core.os.CancellationSignal;
import androidx.core.os.OperationCanceledException;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModelProviders;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.Log;
import app.zxtune.analytics.Analytics;
import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsUtils;

/*
TODO:
 - icons for files
 - per-item progress and error status
 - parallel browsing
*/

public class BrowserModel extends AndroidViewModel {

  private static final String TAG = BrowserModel.class.getName();

  public interface Client {
    void onFileBrowse(Uri uri);

    void onError(String msg);
  }

  public class State {
    @NonNull
    public Uri uri;
    public String query = null;
    @NonNull
    public List<BrowserEntrySimple> breadcrumbs;
    @NonNull
    public List<BrowserEntry> entries;
  }

  private abstract class AsyncTask implements Runnable {

    protected final CancellationSignal signal = new CancellationSignal();

    void checkForCancel() {
      signal.throwIfCanceled();
    }

    final void cancel() {
      signal.cancel();
    }
  }

  private final ExecutorService async = Executors.newSingleThreadExecutor();
  private final MutableLiveData<State> state = new MutableLiveData<>();
  private final MutableLiveData<Integer> progress = new MutableLiveData<>();
  private final AtomicReference<AsyncTask> lastTask = new AtomicReference<>();
  private Client client = new Client() {
    @Override
    public void onFileBrowse(Uri uri) {}

    @Override
    public void onError(String msg) {}
  };

  public static BrowserModel of(Fragment owner) {
    return ViewModelProviders.of(owner).get(BrowserModel.class);
  }

  public BrowserModel(@NonNull Application application) {
    super(application);
  }

  public final LiveData<State> getState() {
    return state;
  }

  public final LiveData<Integer> getProgress() {
    return progress;
  }

  public final void setClient(@NonNull Client client) {
    this.client = client;
  }

  public final void browse(@NonNull Uri uri) {
    Analytics.sendBrowserEvent(uri, Analytics.BROWSER_ACTION_BROWSE);
    executeAsync(new BrowseTask(uri));
  }

  public final void browseParent() {
    final State currentState = state.getValue();
    if (currentState != null && currentState.breadcrumbs.size() > 1) {
      Analytics.sendBrowserEvent(currentState.uri, Analytics.BROWSER_ACTION_BROWSE_PARENT);
      executeAsync(new BrowseParentTask(currentState.breadcrumbs));
    } else {
      Analytics.sendBrowserEvent(Uri.EMPTY, Analytics.BROWSER_ACTION_BROWSE_PARENT);
      executeAsync(new BrowseTask(Uri.EMPTY));
    }
  }

  public final void reload() {
    final State currentState = state.getValue();
    if (currentState != null) {
      browse(currentState.uri);
    }
  }

  public final void search(@NonNull String query) {
    final State currentState = state.getValue();
    if (currentState != null) {
      Analytics.sendBrowserEvent(currentState.uri, Analytics.BROWSER_ACTION_SEARCH);
      executeAsync(new SearchTask(currentState, query));
    }
  }

  private void executeAsync(@NonNull AsyncTask task) {
    final AsyncTask prev = lastTask.getAndSet(task);
    if (prev != null) {
      prev.cancel();
    }
    async.execute(task);
  }

  private class BrowseTask extends AsyncTask {

    private final Uri uri;

    BrowseTask(@NonNull Uri uri) {
      this.uri = uri;
    }

    @Override
    public void run() {
      try {
        progress.postValue(-1);
        final VfsDir dir = resolveDirOrFile(uri, client);
        if (dir != null) {
          checkForCancel();
          final State newState = new State();
          newState.uri = uri;
          newState.entries = getEntries(dir, signal);
          newState.breadcrumbs = getBreadcrumbs(dir);
          state.postValue(newState);
        }
      } catch (OperationCanceledException e) {
        Log.d(TAG, "Canceled browse");
      } catch (Exception e) {
        reportError(e);
      } finally {
        progress.postValue(null);
      }
    }
  }

  class BrowseParentTask extends AsyncTask {

    private final List<BrowserEntrySimple> items;

    BrowseParentTask(List<BrowserEntrySimple> items) {
      this.items = items;
    }

    @Override
    public void run() {
      try {
        progress.postValue(-1);
        for (int idx = items.size() - 2; idx >= 0; --idx) {
          if (tryBrowseAt(idx)) {
            break;
          }
        }
      } catch (OperationCanceledException e) {
        Log.d(TAG, "Canceled browse");
      } finally {
        progress.postValue(null);
      }
    }

    private boolean tryBrowseAt(int idx) {
      try {
        final Uri uri = items.get(idx).uri;
        final VfsDir dir = resolveDir(uri);
        if (dir != null) {
          checkForCancel();
          final State newState = new State();
          newState.uri = uri;
          newState.entries = getEntries(dir, signal);
          newState.breadcrumbs = items.subList(0, idx + 1);
          state.postValue(newState);
          return true;
        }
      } catch (Exception e) {
      }
      return false;
    }
  }

  private class SearchTask extends AsyncTask {

    private final State newState = new State();

    SearchTask(@NonNull State state, @NonNull String query) {
      newState.uri = state.uri;
      newState.breadcrumbs = state.breadcrumbs;
      newState.entries = new ArrayList<>();
      newState.query = query;
    }

    @Override
    public void run() {
      try {
        progress.postValue(-1);
        final VfsDir dir = resolveDir(newState.uri);
        if (dir != null) {
          search(dir);
        }
      } catch (OperationCanceledException e) {
        Log.d(TAG, "Cancelled search");
      } catch (Exception e) {
        reportError(e);
      } finally {
        progress.postValue(null);
      }
    }

    private void search(@NonNull VfsDir dir) throws IOException {
      state.postValue(newState);
      final VfsExtensions.SearchEngine.Visitor visitor = new VfsExtensions.SearchEngine.Visitor() {

        @Override
        public void onFile(VfsFile file) {
          checkForCancel();
          newState.entries.add(createEntry(file));
          state.postValue(newState);
        }
      };
      search(dir, visitor);
    }

    private void search(@NonNull VfsDir dir, @NonNull VfsExtensions.SearchEngine.Visitor visitor) throws IOException {
      final VfsExtensions.SearchEngine engine =
          (VfsExtensions.SearchEngine) dir.getExtension(VfsExtensions.SEARCH_ENGINE);
      if (engine != null) {
        engine.find(newState.query, visitor);
      } else {
        for (final VfsIterator iter = new VfsIterator(new Uri[]{dir.getUri()}); iter.isValid(); iter.next()) {
          checkForCancel();
          final VfsFile file = iter.getFile();
          if (match(file.getName()) || match(file.getDescription())) {
            visitor.onFile(file);
          }
        }
      }
    }

    private boolean match(@NonNull String txt) {
      return txt.toLowerCase().contains(newState.query);
    }
  }

  private void reportError(Exception e) {
    final Throwable cause = e.getCause();
    final String msg = cause != null ? cause.getMessage() : e.getMessage();
    client.onError(msg);
  }

  private static VfsDir resolveDirOrFile(Uri uri, Client client) throws IOException {
    final VfsObject obj = VfsArchive.resolveForced(uri);
    if (obj == null) {
      return null;
    }
    if (obj instanceof VfsDir) {
      final VfsDir dir = (VfsDir) obj;
      if (null == dir.getExtension(VfsExtensions.FEED)) {
        return dir;
      }
      client.onFileBrowse(uri);
    } else if (obj instanceof VfsFile) {
      client.onFileBrowse(uri);
    }
    return null;
  }

  private static VfsDir resolveDir(Uri uri) throws IOException {
    final VfsObject obj = VfsArchive.resolveForced(uri);
    if (obj instanceof VfsDir) {
      return (VfsDir) obj;
    } else {
      return null;
    }
  }

  private static List<BrowserEntry> getEntries(@NonNull VfsDir dir, CancellationSignal signal) throws IOException {
    final ObjectsCollector collector = new ObjectsCollector(signal);
    collector.fill(dir);
    final ArrayList<BrowserEntry> entries = new ArrayList<>(collector.getTotalEntries());
    for (VfsDir d : collector.getDirs()) {
      entries.add(createEntry(d));
    }
    for (VfsFile f : collector.getFiles()) {
      entries.add(createEntry(f));
    }
    return entries;
  }

  private static class ObjectsCollector implements VfsDir.Visitor {

    private final CancellationSignal signal;
    private final ArrayList<VfsDir> dirs = new ArrayList<>();
    private final ArrayList<VfsFile> files = new ArrayList<>();

    ObjectsCollector(CancellationSignal signal) {
      this.signal = signal;
    }

    final void fill(VfsDir dir) throws IOException {
      dir.enumerate(this);
      sort((Comparator<VfsObject>) dir.getExtension(VfsExtensions.COMPARATOR));
    }

    final int getTotalEntries() {
      return dirs.size() + files.size();
    }

    final ArrayList<VfsDir> getDirs() {
      return dirs;
    }

    final ArrayList<VfsFile> getFiles() {
      return files;
    }

    @Override
    public void onItemsCount(int count) {
      signal.throwIfCanceled();
      dirs.ensureCapacity(count);
      files.ensureCapacity(count);
    }

    @Override
    public void onDir(VfsDir dir) {
      signal.throwIfCanceled();
      dirs.add(dir);
    }

    @Override
    public void onFile(VfsFile file) {
      signal.throwIfCanceled();
      files.add(file);
    }

    private void sort(Comparator<VfsObject> cmp) {
      if (cmp == null) {
        sort(DefaultComparator.instance());
      } else {
        Collections.sort(dirs, cmp);
        Collections.sort(files, cmp);
      }
    }
  }

  private static List<BrowserEntrySimple> getBreadcrumbs(@NonNull VfsDir dir) {
    if (Uri.EMPTY.equals(dir.getUri())) {
      return Collections.<BrowserEntrySimple>emptyList();
    } else {
      final ArrayList<BrowserEntrySimple> breadcrumbs = new ArrayList<>();
      for (VfsObject obj = dir; obj != null; obj = obj.getParent()) {
        breadcrumbs.add(createBreadcrumb(obj));
      }
      Collections.reverse(breadcrumbs);
      return breadcrumbs;
    }
  }

  private static BrowserEntrySimple createBreadcrumb(@NonNull VfsObject obj) {
    final BrowserEntrySimple result = new BrowserEntrySimple();
    result.uri = obj.getUri();
    result.title = obj.getName();
    result.icon = VfsUtils.getObjectIcon(obj);
    return result;
  }

  private static BrowserEntry createEntry(@NonNull VfsDir dir) {
    final BrowserEntry result = createEntry((VfsObject) dir);
    result.type = BrowserEntry.FOLDER;
    result.details = "";
    return result;
  }

  private static BrowserEntry createEntry(@NonNull VfsFile file) {
    final BrowserEntry result = createEntry((VfsObject) file);
    result.type = BrowserEntry.FILE;
    result.details = file.getSize();
    return result;
  }

  private static BrowserEntry createEntry(@NonNull VfsObject obj) {
    final BrowserEntry result = new BrowserEntry();
    result.uri = obj.getUri();
    result.title = obj.getName();
    result.description = obj.getDescription();
    result.icon = VfsUtils.getObjectIcon(obj);
    return result;
  }
}
