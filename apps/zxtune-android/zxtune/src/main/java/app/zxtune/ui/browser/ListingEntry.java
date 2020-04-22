package app.zxtune.ui.browser;

import android.net.Uri;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;

import java.lang.annotation.Retention;

import app.zxtune.R;

import static java.lang.annotation.RetentionPolicy.SOURCE;

// public for binding
public class ListingEntry {
  @Retention(SOURCE)
  @IntDef({FOLDER, FILE})
  @interface Type {}

  public static final int FOLDER = 0;
  public static final int FILE = 1;

  Uri uri;
  @Type
  public int type;
  @DrawableRes
  public int icon;
  public String title;
  public String description;
  public String details;
  public Integer tracks;
  public Boolean cached;

  @DrawableRes
  public int getDisplayIcon() {
    if (icon != 0) {
      return icon;
    } else if (type == FOLDER) {
      return R.drawable.ic_browser_folder;
    } else if (cached != null && !cached) {
      return R.drawable.ic_browser_file_remote;
    } else if (tracks == null) {
      return R.drawable.ic_browser_file_unknown;
    } else if (tracks == 0) {
      return R.drawable.ic_browser_file_unsupported;
    } else if (tracks == 1) {
      return R.drawable.ic_browser_file_track;
    } else {
      return R.drawable.ic_browser_file_archive;
    }
  }
}
