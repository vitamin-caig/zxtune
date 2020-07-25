package app.zxtune.ui.browser;

import android.net.Uri;

import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;

// public for binding
public class BreadcrumbsEntry {
  Uri uri;
  public String title;
  @DrawableRes
  @Nullable
  public Integer icon;

  BreadcrumbsEntry(Uri uri, String title, @Nullable Integer icon) {
    this.uri = uri;
    this.title = title;
    this.icon = icon;
  }
}
