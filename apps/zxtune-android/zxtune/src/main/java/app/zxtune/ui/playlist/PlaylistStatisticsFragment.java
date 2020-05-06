/**
 * @file
 * @brief Dialog fragment with playlist statistics
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui.playlist;

import android.app.Application;
import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.widget.ArrayAdapter;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProviders;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import app.zxtune.R;
import app.zxtune.analytics.Analytics;
import app.zxtune.playlist.ProviderClient;
import app.zxtune.playlist.Statistics;

public class PlaylistStatisticsFragment extends DialogFragment {

  private static final String IDS_KEY = "ids";

  static DialogFragment createInstance(@Nullable long[] ids) {
    final DialogFragment res = new PlaylistStatisticsFragment();
    final Bundle args = new Bundle();
    args.putLongArray(IDS_KEY, ids);
    res.setArguments(args);
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_STATISTICS, ids != null ? ids.length : 0);
    return res;
  }

  @Override
  public Dialog onCreateDialog(@NonNull Bundle savedInstanceState) {
    final Context ctx = getActivity();

    final ArrayAdapter<String> adapter = new ArrayAdapter<>(ctx, android.R.layout.simple_list_item_1);
    final AlertDialog result = new AlertDialog.Builder(ctx)
        .setTitle(R.string.statistics)
        .setAdapter(adapter, null)
        .create();
    final Model model = Model.of(this);
    model.getData().observe(this, new Observer<Statistics>() {
      @Override
      public void onChanged(Statistics stat) {
        adapter.clear();
        if (stat != null) {
          final String tracks = getResources().getQuantityString(R.plurals.tracks, stat.getTotal(), stat.getTotal());
          final String duration = stat.getDuration().toString();
          adapter.add(getString(R.string.statistics_tracks) + ": " + tracks);
          adapter.add(getString(R.string.statistics_duration) + ": " + duration);
        } else {
          adapter.add("Failed to load...");
        }
      }
    });
    final Bundle args = getArguments();
    final long[] ids = args != null ? args.getLongArray(IDS_KEY) : null;
    model.load(ids);
    return result;
  }

  public static class Model extends AndroidViewModel {
    private final ProviderClient client;
    private final ExecutorService async;
    private MutableLiveData<Statistics> data;

    static Model of(Fragment owner) {
      return ViewModelProviders.of(owner).get(Model.class);
    }

    public Model(@NonNull Application application) {
      super(application);
      this.client = new ProviderClient(application);
      this.async = Executors.newSingleThreadExecutor();
      this.data = new MutableLiveData<>();
    }

    final LiveData<Statistics> getData() {
      return data;
    }

    final void load(@Nullable final long[] ids) {
      async.execute(new Runnable() {
        @Override
        public void run() {
          data.postValue(client.statistics(ids));
        }
      });
    }
  }
}
