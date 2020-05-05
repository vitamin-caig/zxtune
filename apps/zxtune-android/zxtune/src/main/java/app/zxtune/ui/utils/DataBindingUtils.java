package app.zxtune.ui.utils;

import android.widget.ImageView;

import androidx.annotation.DrawableRes;
import androidx.databinding.BindingAdapter;

public class DataBindingUtils {
  @BindingAdapter({"resource"})
  public static void setImageResource(ImageView view, @DrawableRes int res) {
    view.setImageResource(res);
  }
}
