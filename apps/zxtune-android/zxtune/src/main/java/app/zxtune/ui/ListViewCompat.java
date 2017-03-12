/**
 *
 * @file
 *
 * @brief ListView implementation with modal choice mode support even for old API
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.util.SparseBooleanArray;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ListView;

import app.zxtune.R;

public class ListViewCompat extends ListView {

  public interface ActionMode {

    MenuInflater getMenuInflater();

    void setTitle(String title);

    void finish();
  }

  public interface MultiChoiceModeListener {

    boolean onCreateActionMode(ActionMode mode, Menu menu);

    boolean onPrepareActionMode(ActionMode mode, Menu menu);

    boolean onActionItemClicked(ActionMode mode, MenuItem item);

    void onDestroyActionMode(ActionMode mode);

    void onItemCheckedStateChanged(ActionMode mode, int position, long id, boolean checked);
  }

  private static final boolean hasNativeModalSelection =
      Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB;

  private android.support.v7.view.ActionMode actionModeCompat;

  public ListViewCompat(Context context) {
    super(context);
  }

  public ListViewCompat(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  public ListViewCompat(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
  }
  
  public final void storeViewPosition(int pos) {
    setTag(Integer.valueOf(pos));
  }
  
  public final void useStoredViewPosition() {
    final Integer pos = (Integer) getTag();
    if (pos != null) {
      setSelection(pos);
      setTag(null);
    }
  }

  public final boolean processActionItemClick(int itemId) {
    switch (itemId) {
      case R.id.action_select_all:
        selectAll();
        break;
      case R.id.action_select_none:
        selectNone();
        break;
      case R.id.action_select_invert:
        invertSelection();
        break;
      default:
        return false;
    }
    return true;
  }

  private void selectAll() {
    for (int i = 0, lim = getAdapter().getCount(); i != lim; ++i) {
      setItemChecked(i, true);
    }
  }

  private void selectNone() {
    for (int i = 0, lim = getAdapter().getCount(); i != lim; ++i) {
      setItemChecked(i, false);
    }
  }

  private void invertSelection() {
    for (int i = 0, lim = getAdapter().getCount(); i != lim; ++i) {
      setItemChecked(i, !isItemChecked(i));
    }
  }

  public final int getCheckedItemsCount() {
    return hasNativeModalSelection ? getCheckedItemsCountNative() : getCheckedItemsCountSupp();
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private int getCheckedItemsCountNative() {
    return super.getCheckedItemCount();
  }

  private int getCheckedItemsCountSupp() {
    final SparseBooleanArray checked = getCheckedItemPositions();
    int count = 0;
    for (int i = 0, lim = checked.size(); i != lim; ++i) {
      if (checked.valueAt(i)) {
        ++count;
      }
    }
    return count;
  }

  @Override
  public void setOnItemClickListener(OnItemClickListener listener) {
    final ClickListener prev = (ClickListener) super.getOnItemClickListener();
    if (prev != null) {
      prev.setDelegate(listener);
    } else {
      super.setOnItemClickListener(listener);
    }
  }

  public final void setMultiChoiceModeListener(MultiChoiceModeListener listener) {
    if (hasNativeModalSelection) {
      setMultiChoiceModeListenerNative(listener);
    } else {
      setMultiChoiceModeListenerSupp(listener);
    }
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private void setMultiChoiceModeListenerNative(MultiChoiceModeListener listener) {
    super.setChoiceMode(CHOICE_MODE_MULTIPLE_MODAL);
    super.setMultiChoiceModeListener(new NativeMultiChoiceModeListener(listener));
  }

  private void setMultiChoiceModeListenerSupp(MultiChoiceModeListener listener) {
    super.setChoiceMode(CHOICE_MODE_NONE);
    final ClickListener cur = new ClickListener(listener);
    cur.setDelegate(super.getOnItemClickListener());
    super.setOnItemClickListener(cur);
    super.setOnItemLongClickListener(new LongClickListener(listener));
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private static class NativeMultiChoiceModeListener implements AbsListView.MultiChoiceModeListener {

    private final MultiChoiceModeListener delegate;

    public NativeMultiChoiceModeListener(MultiChoiceModeListener listener) {
      this.delegate = listener;
    }

    @Override
    public boolean onActionItemClicked(android.view.ActionMode mode, MenuItem item) {
      return delegate.onActionItemClicked(new ActionModeNative(mode), item);
    }

    @Override
    public boolean onCreateActionMode(android.view.ActionMode mode, Menu menu) {
      return delegate.onCreateActionMode(new ActionModeNative(mode), menu);
    }

    @Override
    public void onDestroyActionMode(android.view.ActionMode mode) {
      delegate.onDestroyActionMode(new ActionModeNative(mode));
    }

    @Override
    public boolean onPrepareActionMode(android.view.ActionMode mode, Menu menu) {
      return delegate.onPrepareActionMode(new ActionModeNative(mode), menu);
    }

    @Override
    public void onItemCheckedStateChanged(android.view.ActionMode mode, int position, long id,
        boolean checked) {
      delegate.onItemCheckedStateChanged(new ActionModeNative(mode), position, id, checked);
    }
  }

  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private static class ActionModeNative implements ActionMode {

    private final android.view.ActionMode mode;

    ActionModeNative(android.view.ActionMode mode) {
      this.mode = mode;
    }

    @Override
    public MenuInflater getMenuInflater() {
      return mode.getMenuInflater();
    }

    @Override
    public void setTitle(String title) {
      mode.setTitle(title);
    }

    @Override
    public void finish() {
      mode.finish();
    }
  }

  private class ClickListener implements OnItemClickListener {

    private final MultiChoiceModeListener listener;
    private OnItemClickListener delegate;

    ClickListener(MultiChoiceModeListener listener) {
      this.listener = listener;
    }

    final void setDelegate(OnItemClickListener delegate) {
      this.delegate = delegate;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
      if (actionModeCompat != null) {
        listener.onItemCheckedStateChanged(new ActionModeSupp(actionModeCompat), position, id,
            isItemChecked(position));
      } else {
        delegate.onItemClick(parent, view, position, id);
      }
    }
  }

  private class LongClickListener implements OnItemLongClickListener {

    private final MultiChoiceModeListener listener;

    LongClickListener(MultiChoiceModeListener listener) {
      this.listener = listener;
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
      if (actionModeCompat != null) {
        return false;
      }
      final android.support.v7.app.ActionBarActivity activity =
          (android.support.v7.app.ActionBarActivity) getContext();
      actionModeCompat = activity.startSupportActionMode(new ActionModeCallback(listener));
      parent.performItemClick(view, position, id);
      return true;
    }
  }

  private class ActionModeCallback implements android.support.v7.view.ActionMode.Callback {

    private final MultiChoiceModeListener listener;

    ActionModeCallback(MultiChoiceModeListener listener) {
      this.listener = listener;
    }

    @Override
    public boolean onCreateActionMode(android.support.v7.view.ActionMode mode, Menu menu) {
      ListViewCompat.this.setChoiceMode(CHOICE_MODE_MULTIPLE);
      return listener.onCreateActionMode(new ActionModeSupp(mode), menu);
    }

    @Override
    public void onDestroyActionMode(android.support.v7.view.ActionMode mode) {
      listener.onDestroyActionMode(new ActionModeSupp(mode));
      actionModeCompat = null;
      ListViewCompat.this.selectNone();
      ListViewCompat.this.post(new Runnable() {
        @Override
        public void run() {
          ListViewCompat.this.setChoiceMode(CHOICE_MODE_NONE);
        }
      });
    }

    @Override
    public boolean onPrepareActionMode(android.support.v7.view.ActionMode mode, Menu menu) {
      return listener.onPrepareActionMode(new ActionModeSupp(mode), menu);
    }

    @Override
    public boolean onActionItemClicked(android.support.v7.view.ActionMode mode, MenuItem item) {
      return listener.onActionItemClicked(new ActionModeSupp(mode), item);
    }
  }

  private static class ActionModeSupp implements ActionMode {

    private final android.support.v7.view.ActionMode mode;

    ActionModeSupp(android.support.v7.view.ActionMode mode) {
      this.mode = mode;
    }

    @Override
    public MenuInflater getMenuInflater() {
      return mode.getMenuInflater();
    }

    @Override
    public void setTitle(String title) {
      mode.setTitle(title);
    }

    @Override
    public void finish() {
      mode.finish();
    }
  }
}
