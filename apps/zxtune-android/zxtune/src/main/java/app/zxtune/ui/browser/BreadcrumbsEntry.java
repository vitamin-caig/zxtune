package app.zxtune.ui.browser;

import android.net.Uri;

import androidx.annotation.DrawableRes;

// public for binding
public class BreadcrumbsEntry {
  Uri uri;
  @DrawableRes
  public int icon;
  public String title;
}
