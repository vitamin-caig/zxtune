/*
 * @file
 * 
 * @brief Browser ui class
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.drawable.BitmapDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.PopupWindow;
import app.zxtune.PlaybackService;
import app.zxtune.R;
import app.zxtune.fs.Vfs;

public class BrowserFragment extends Fragment
    implements
      BreadCrumbsUriView.OnUriSelectionListener,
      DirView.OnEntryClickListener {

  private static final String LOG = BrowserFragment.class.getName();
  private SavedState state;
  private View sources;
  private BreadCrumbsUriView position;
  private DirView listing;

  public static Fragment createInstance() {
    return new BrowserFragment();
  }
  
  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);
    state = new SavedState(PreferenceManager.getDefaultSharedPreferences(activity));
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    return container != null ? inflater.inflate(R.layout.browser, container, false) : null;
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState) {
    super.onViewCreated(view, savedInstanceState);
    sources = view.findViewById(R.id.browser_sources);
    sources.setOnClickListener(new SourcesClickListener());
    position = (BreadCrumbsUriView) view.findViewById(R.id.browser_breadcrumb);
    position.setOnUriSelectionListener(this);
    listing = (DirView) view.findViewById(R.id.browser_content);
    listing.setOnEntryClickListener(this);
  }

  @Override
  public void onStart() {
    super.onStart();
    final Uri lastPath = state.getCurrentPath();
    setNewState(lastPath);
  }
  
  @Override
  public void onStop() {
    super.onStop();
    storeCurrentState();
  }

  @Override
  public void onUriSelection(Uri uri) {
    setCurrentPath(uri);
  }

  @Override
  public void onFileClick(Uri uri) {
    final Context context = getActivity();
    final Intent intent = new Intent(Intent.ACTION_VIEW, uri, context, PlaybackService.class);
    context.startService(intent);
  }

  @Override
  public void onDirClick(Uri uri) {
    setCurrentPath(uri);
  }

  @Override
  public boolean onFileLongClick(Uri uri) {
    //TODO
    final Context context = getActivity();
    final Intent intent = new Intent(Intent.ACTION_INSERT, uri, context, PlaybackService.class);
    context.startService(intent);
    return true;
  }

  @Override
  public boolean onDirLongClick(Uri uri) {
    return false;
  }

  private final void setCurrentPath(Uri uri) {
    storeCurrentState();
    setNewState(uri);
  }
  
  private final void storeCurrentState() {
    state.setCurrentViewPosition(listing.getFirstVisiblePosition());
  }
  
  private final void setNewState(Uri uri) {
    Log.d(LOG, "Set current path to " + uri);
    state.setCurrentPath(uri);
    position.setUri(uri);
    listing.setUri(uri);
    listing.setSelection(state.getCurrentViewPosition());
  }

  private class SourcesClickListener extends DirView.StubOnEntryClickListener
      implements
        View.OnClickListener {

    PopupWindow popup;

    @Override
    public void onClick(View v) {
      final Context context = v.getContext();
      final DirView view = new DirView(context);
      final View root = View.inflate(context, R.layout.popup, null);
      final ViewGroup rootLayout = (ViewGroup) root.findViewById(R.id.popup_layout);
      rootLayout.addView(view);
      view.setDir(Vfs.getRoot());
      view.setOnEntryClickListener(this);

      popup =
          new PopupWindow(root, WindowManager.LayoutParams.WRAP_CONTENT,
              WindowManager.LayoutParams.WRAP_CONTENT, true);
      popup.setBackgroundDrawable(new BitmapDrawable());
      popup.setTouchable(true);
      popup.setOutsideTouchable(true);
      popup.showAsDropDown(v);
    }

    public void onDirClick(Uri uri) {
      popup.dismiss();
      BrowserFragment.this.onDirClick(uri);
    }
  }
  
  private static class SavedState {
    
    private final static String PREF_BROWSER = "browser_";
    private final static String PREF_BROWSER_CURRENT = PREF_BROWSER + "current";
    private final static String PREF_BROWSER_PATH_TEMPLATE = PREF_BROWSER + "%d_path";
    private final static String PREF_BROWSER_VIEWPOS_TEMPLATE = PREF_BROWSER + "%d_viewpos";
    private final SharedPreferences prefs;
    private PathAndPosition current;
    
    public SavedState(SharedPreferences prefs) {
      this.prefs = prefs;
      this.current = new PathAndPosition();
    }
    
    public Uri getCurrentPath() {
      return Uri.parse(current.getPath());
    }
    
    public void setCurrentPath(Uri uri) {
      final String curPath = current.getPath();
      final String newPath = uri.toString();
      if (curPath.equals(newPath)) {
        return;
      } else if (isNested(curPath, newPath)) {
        setInnerPath(newPath);
      } else if (isNested(newPath, curPath)) {
        setOuterPath(newPath);
      } else {
        setPath(newPath);
      }
      current.store();
    }
    
    public int getCurrentViewPosition() {
      return current.getViewPosition();
    }
    
    public void setCurrentViewPosition(int pos) {
      current.setViewPosition(pos);
    }
    
    private void setInnerPath(String newPath) {
      current = new PathAndPosition(current.getIndex() + 1, newPath);
    }
    
    private void setOuterPath(String newPath) {
      current = findByPath(newPath);
    }
    
    private void setPath(String newPath) {
      current = new PathAndPosition(0, newPath);
    }
    
    private PathAndPosition findByPath(String newPath) {
      for (int idx = current.getIndex() - 1; idx >= 0; --idx) {
        final PathAndPosition pos = new PathAndPosition(idx);
        if (newPath.equals(pos.getPath())) {
          return pos;
        }
      }
      return new PathAndPosition(0, newPath);
    }
    
    // checks if rh is nested path relative to lh
    private static final boolean isNested(String lh, String rh) {
      return rh.startsWith(lh);
    }
    
    private class PathAndPosition {
      
      private final int index;
      private String path;
      private int position;
      
      public PathAndPosition() {
        this(prefs.getInt(PREF_BROWSER_CURRENT, 0));
      }
      
      public PathAndPosition(int idx) {
        this.index = idx;
        this.path = prefs.getString(getPathKey(), "");
        this.position = prefs.getInt(getPosKey(), 0);
      }
      
      public PathAndPosition(int idx, String path) {
        this.index = idx;
        this.path = path;
        this.position = 0;
      }
      
      public int getIndex() {
        return index;
      }
      
      public String getPath() {
        return path;
      }
      
      public int getViewPosition() {
        return position;
      }
      
      public void setViewPosition(int newPos) {
        if (newPos != position) {
          position = newPos;
          store();
        }
      }
      
      private void store() {
        final Editor editor = prefs.edit();
        editor.putString(getPathKey(), this.path);
        editor.putInt(getPosKey(), this.position);
        editor.putInt(PREF_BROWSER_CURRENT, index);
        editor.commit();
      }
      
      private String getPathKey() {
        return String.format(PREF_BROWSER_PATH_TEMPLATE, index);
      }
      
      private String getPosKey() {
        return String.format(PREF_BROWSER_VIEWPOS_TEMPLATE, index);
      }
    }
  }
}
