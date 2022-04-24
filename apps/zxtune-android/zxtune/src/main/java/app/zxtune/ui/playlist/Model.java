package app.zxtune.ui.playlist;

import android.app.Application;
import android.database.Cursor;

import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModelProvider;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;
import app.zxtune.playlist.Database;
import app.zxtune.playlist.ProviderClient;

// public for provider
public class Model extends AndroidViewModel {

  private final ProviderClient client;
  private final ExecutorService async;
  @Nullable
  private MutableLiveData<List<Entry>> items;

  static Model of(Fragment owner) {
    return new ViewModelProvider(owner,
        ViewModelProvider.AndroidViewModelFactory.getInstance(owner.getActivity().getApplication())).get(Model.class);
  }

  public Model(Application application) {
    this(application, new ProviderClient(application), Executors.newSingleThreadExecutor());
  }

  @VisibleForTesting
  Model(Application application, ProviderClient client, ExecutorService async) {
    super(application);
    this.client = client;
    this.async = async;
    client.registerObserver(() -> {
      if (items != null) {
        loadAsync();
      }
    });
  }

  @Override
  public void onCleared() {
    client.unregisterObserver();
  }

  final LiveData<List<Entry>> getItems() {
    if (items == null) {
      items = new MutableLiveData<>();
      loadAsync();
    }
    return items;
  }

  private void loadAsync() {
    async.execute(this::load);
  }

  private void load() {
    final Cursor cursor = client.query(null);
    if (cursor != null) {
      try {
        final ArrayList<Entry> items = new ArrayList<>(cursor.getCount());
        while (cursor.moveToNext()) {
          items.add(createItem(cursor));
        }
        this.items.postValue(items);
      } finally {
        cursor.close();
      }
    }
  }

  private static Entry createItem(Cursor cursor) {
    return new Entry(
        cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal()),
        Identifier.parse(cursor.getString(Database.Tables.Playlist.Fields.location.ordinal())),
        cursor.getString(Database.Tables.Playlist.Fields.title.ordinal()),
        cursor.getString(Database.Tables.Playlist.Fields.author.ordinal()),
        TimeStamp.fromMilliseconds(cursor.getLong(Database.Tables.Playlist.Fields.duration.ordinal()))
    );
  }
}
