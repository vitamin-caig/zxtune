package app.zxtune.ui.utils;

import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.view.ActionMode;
import androidx.recyclerview.selection.Selection;
import androidx.recyclerview.selection.SelectionTracker;

import java.util.ArrayList;
import java.util.List;

import app.zxtune.R;

final public class SelectionUtils<T> extends SelectionTracker.SelectionObserver<T> {

  public interface Client<T> {
    String getTitle(int count);

    List<T> getAllItems();

    void fillMenu(MenuInflater inflater, Menu menu);

    boolean processMenu(int itemId, Selection<T> selection);
  }

  private final AppCompatActivity activity;
  private final SelectionTracker<T> selectionTracker;
  private final Client client;
  @Nullable
  private ActionMode action;

  public static <T> void install(AppCompatActivity activity, SelectionTracker<T> selectionTracker,
                      Client client) {
    final SelectionTracker.SelectionObserver<T> observer = new SelectionUtils<>(activity,
                                                                             selectionTracker,
        client);
    selectionTracker.addObserver(observer);
  }

  private SelectionUtils(AppCompatActivity activity, SelectionTracker<T> selectionTracker,
                 Client client) {
    this.activity = activity;
    this.selectionTracker = selectionTracker;
    this.client = client;
  }

  @Override
  public void onSelectionChanged() {
    final int count = selectionTracker.getSelection().size();
    if (count != 0) {
      if (action == null) {
        action = activity.startSupportActionMode(new ActionModeCallback());
      }
      if (action != null) {
        action.setTitle(client.getTitle(count));
      }
    } else if (action != null) {
      action.finish();
      action = null;
    }
  }

  @Override
  public void onSelectionRestored() {
    onSelectionChanged();
  }

  private class ActionModeCallback implements ActionMode.Callback {
    @Override
    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
      final MenuInflater inflater = mode.getMenuInflater();
      inflater.inflate(R.menu.selection, menu);
      client.fillMenu(inflater, menu);
      return true;
    }

    @Override
    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
      return false;
    }

    @Override
    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
      if (processActionItemClick(item.getItemId())) {
        return true;
      } else if (client.processMenu(item.getItemId(), selectionTracker.getSelection())) {
        mode.finish();
        return true;
      } else {
        return false;
      }
    }

    @Override
    public void onDestroyActionMode(ActionMode mode) {
      selectionTracker.clearSelection();
    }
  }

  private boolean processActionItemClick(int itemId) {
    switch (itemId) {
      case R.id.action_select_all:
        selectionTracker.setItemsSelected(client.getAllItems(), true);
        break;
      case R.id.action_select_none:
        selectionTracker.clearSelection();
        break;
      case R.id.action_select_invert:
        invertSelection();
        break;
      default:
        return false;
    }
    return true;
  }

  private void invertSelection() {
    final List<T> all = client.getAllItems();
    final ArrayList<T> toSelect = new ArrayList<>(all.size());
    final ArrayList<T> toUnselect = new ArrayList<>(all.size());
    for (T id : all) {
      if (selectionTracker.isSelected(id)) {
        toUnselect.add(id);
      } else {
        toSelect.add(id);
      }
    }
    selectionTracker.setItemsSelected(toSelect, true);
    selectionTracker.setItemsSelected(toUnselect, false);
  }
}
