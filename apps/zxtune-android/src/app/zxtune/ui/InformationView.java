/**
 * @file
 * @brief Information view logic
 * @version $Id:$
 * @author
 */
package app.zxtune.ui;

import android.content.res.Resources;
import android.net.Uri;
import android.text.Html;
import android.text.TextUtils;
import android.view.View;
import android.widget.HorizontalScrollView;
import android.widget.TextView;
import app.zxtune.R;
import app.zxtune.playback.Item;
import app.zxtune.playback.ItemStub;

class InformationView {

  private final static String FORMAT = "<b>%1$s:</b> %2$s";
  private final static String LINEBREAK = "<br>";

  private final HorizontalScrollView frame;
  private final TextView content;
  private final String titleField;
  private final String authorField;
  private final String programField;
  private final String commentField;
  private final String locationField;

  public InformationView(View layout) {
    this.frame = (HorizontalScrollView) layout.findViewById(R.id.information_frame);
    this.content = (TextView) layout.findViewById(R.id.information_content);
    final Resources res = layout.getResources();
    this.titleField = res.getString(R.string.information_title);
    this.authorField = res.getString(R.string.information_author);
    this.programField = res.getString(R.string.information_program);
    this.commentField = res.getString(R.string.information_comment);
    this.locationField = res.getString(R.string.information_location);
    update(ItemStub.instance());
  }

  public void update(Item item) {
    final StringBuilder builder = new StringBuilder();
    addNonEmptyField(builder, titleField, item.getTitle());
    addNonEmptyField(builder, authorField, item.getAuthor());
    addFallbackField(builder, locationField, Uri.decode(item.getDataId().toString()));
    addNonEmptyField(builder, programField, item.getProgram());
    addNonEmptyField(builder, commentField, item.getComment());
    final CharSequence styledVal = Html.fromHtml(builder.toString());
    content.setText(styledVal);
    frame.scrollTo(0, 0);
  }
  
  private void addNonEmptyField(StringBuilder builder, String fieldName, String fieldValue) {
    if (0 != fieldValue.length()) {
      addField(builder, fieldName, fieldValue);
    }
  }
  
  private void addFallbackField(StringBuilder builder, String fieldName, String fieldValue) {
    if (0 == builder.length() && 0 != fieldValue.length()) {
      addField(builder, fieldName, fieldValue);
    }
  }
  
  private void addField(StringBuilder builder, String fieldName, String fieldValue) {
    if (0 != builder.length()) {
      builder.append(LINEBREAK);
    }
    final String escapedValue = TextUtils.htmlEncode(fieldValue);
    builder.append(String.format(FORMAT, fieldName, escapedValue));
  }
}
