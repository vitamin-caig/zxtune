package app.zxtune.ui.browser;

import android.net.Uri;

import androidx.annotation.DrawableRes;

// public for binding
public class BreadcrumbsEntry {
  Uri uri;
  public String title;
  @DrawableRes
  public int icon;

  BreadcrumbsEntry(Uri uri, String title, int icon) {
    this.uri = uri;
    this.title = title;
    this.icon = icon;
  }
}
