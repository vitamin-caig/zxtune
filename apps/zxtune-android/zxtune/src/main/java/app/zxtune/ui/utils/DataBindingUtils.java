package app.zxtune.ui.utils;

import android.text.TextUtils;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;
import androidx.databinding.BindingAdapter;

import kotlin.text.StringsKt;

public class DataBindingUtils {
  @BindingAdapter({"resource"})
  public static void setImageResource(ImageView view, @DrawableRes int res) {
    view.setImageResource(res);
  }

  @BindingAdapter({"text"})
  public static void setText(TextView view, @Nullable CharSequence txt) {
    view.setText(txt);
    view.setVisibility(TextUtils.isEmpty(txt) || StringsKt.isBlank(txt) ? TextView.GONE : TextView.VISIBLE);
  }
}
