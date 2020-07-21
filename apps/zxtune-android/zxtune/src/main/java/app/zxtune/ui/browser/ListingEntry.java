package app.zxtune.ui.browser;

import android.net.Uri;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;
import androidx.annotation.Nullable;

import java.lang.annotation.Retention;

import app.zxtune.R;

import static java.lang.annotation.RetentionPolicy.SOURCE;

// public for binding
public class ListingEntry {
  @Retention(SOURCE)
  @IntDef({FOLDER, FILE})
  private @interface Type {}

  public static final int FOLDER = 0;
  public static final int FILE = 1;

  Uri uri;
  @Type
  public int type;
  @DrawableRes
  @Nullable
  public Integer icon;
  public String title;
  public String description;
  @Nullable
  public String details;
  @Nullable
  public Integer tracks;
  @Nullable
  public Boolean cached;

  ListingEntry(Uri uri, String title, String description) {
    this.uri = uri;
    this.title = title;
    this.description = description;
  }

  @DrawableRes
  public int getDisplayIcon() {
    if (icon != null) {
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
