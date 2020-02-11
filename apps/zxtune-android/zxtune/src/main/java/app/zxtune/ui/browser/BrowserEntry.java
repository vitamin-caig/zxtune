package app.zxtune.ui.browser;

import android.net.Uri;

import androidx.annotation.DrawableRes;
import androidx.annotation.IntDef;

import java.lang.annotation.Retention;

import static java.lang.annotation.RetentionPolicy.SOURCE;

public class BrowserEntry {
  @Retention(SOURCE)
  @IntDef({FOLDER, FILE})
  @interface Type {}

  public static final int FOLDER = 0;
  public static final int FILE = 1;

  public Uri uri;
  @Type
  public int type;
  @DrawableRes
  public int icon;
  public String title;
  public String description;
  public String details;
}
