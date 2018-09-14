package app.zxtune.ui.utils;

import android.content.Context;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import app.zxtune.R;

public final class ListItemViewHolder {

  private final ImageView icon;
  private final TextView main;
  private final TextView aux;
  private final TextView detail;

  private ListItemViewHolder(View view) {
    this.icon = (ImageView) view.findViewById(R.id.list_item_icon);
    this.main = (TextView) view.findViewById(R.id.list_item_text_main);
    this.aux = (TextView) view.findViewById(R.id.list_item_text_aux);
    this.detail = (TextView) view.findViewById(R.id.list_item_text_detail);
  }

  public final void setIcon(@Nullable Integer res) {
    if (res != null) {
      icon.setImageResource(res);
      icon.setVisibility(View.VISIBLE);
    } else {
      icon.setVisibility(View.GONE);
    }
  }

  public final void setMainText(String text) {
    setText(main, text);
  }
  
  public final void setAuxText(String text) {
    setText(aux, text);
  }

  public void setDetailText(@Nullable String text) {
    setText(detail, text);
  }

  private static void setText(TextView view, @Nullable String text) {
    if (text == null || text.isEmpty()) {
      view.setVisibility(View.GONE);
    } else {
      view.setText(text);
      view.setVisibility(View.VISIBLE);
    }
  }
  
  public static View createView(ViewGroup parent) {
    final View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.list_item, parent, false);
    view.setTag(new ListItemViewHolder(view));
    return view;
  }

  public static ListItemViewHolder fromView(View view) {
    return (ListItemViewHolder) view.getTag();
  }
}
