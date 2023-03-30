/**
 * @file
 * @brief Information view logic
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.net.Uri;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import android.text.Html;
import android.text.TextUtils;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;

import app.zxtune.R;
import app.zxtune.core.ModuleAttributes;
import app.zxtune.device.media.MediaModel;

class InformationView {

  private static final String FORMAT = "<b><big>%1$s:</big></b><br>&nbsp;%2$s";
  private static final String RAW_FORMAT = "<br><tt>%1$s</tt>";
  private static final String LINEBREAK = "<br>";

  private final TextView content;
  private final String titleField;
  private final String authorField;
  private final String programField;
  private final String commentField;
  private final String locationField;

  InformationView(FragmentActivity activity, View layout) {
    this.content = layout.findViewById(R.id.information_content);
    this.titleField = activity.getString(R.string.information_title);
    this.authorField = activity.getString(R.string.information_author);
    this.programField = activity.getString(R.string.information_program);
    this.commentField = activity.getString(R.string.information_comment);
    this.locationField = activity.getString(R.string.information_location);
    this.content.setMovementMethod(ScrollingMovementMethod.getInstance());
    final MediaModel model = MediaModel.of(activity);
    model.getMetadata().observe(activity, mediaMetadataCompat -> {
      if (mediaMetadataCompat != null) {
        update(mediaMetadataCompat);
        content.setEnabled(true);
      } else {
        content.setEnabled(false);
      }
    });
  }

  private void update(MediaMetadataCompat metadata) {
    final MediaDescriptionCompat description = metadata.getDescription();
    final StringBuilder builder = new StringBuilder();
    addNonEmptyField(builder, locationField, description.getMediaUri());
    addNonEmptyField(builder, titleField, description.getTitle());
    addNonEmptyField(builder, authorField, description.getSubtitle());
    addNonEmptyField(builder, programField, metadata.getString(ModuleAttributes.PROGRAM));
    addNonEmptyField(builder, commentField, metadata.getString(ModuleAttributes.COMMENT));
    addNonEmptyRawField(builder, metadata.getString(ModuleAttributes.STRINGS));
    final CharSequence styledVal = Html.fromHtml(builder.toString());
    content.setText(styledVal);
    content.scrollTo(0, 0);
  }

  private void addNonEmptyField(StringBuilder builder, String fieldName, @Nullable CharSequence fieldValue) {
    if (!TextUtils.isEmpty(fieldValue)) {
      addField(builder, fieldName, fieldValue);
    }
  }

  private void addNonEmptyField(StringBuilder builder, String fieldName, @Nullable Uri fieldValue) {
    if (fieldValue != null) {
      addNonEmptyField(builder, fieldName, Uri.decode(fieldValue.toString()));
    }
  }

  private void addField(StringBuilder builder, String fieldName, CharSequence fieldValue) {
    if (0 != builder.length()) {
      builder.append(LINEBREAK);
    }
    final String escapedValue = TextUtils.htmlEncode(fieldValue.toString());
    builder.append(String.format(FORMAT, fieldName, escapedValue));
  }

  private void addNonEmptyRawField(StringBuilder builder, @Nullable String fieldValue) {
    if (!TextUtils.isEmpty(fieldValue)) {
      addRawField(builder, fieldValue);
    }
  }

  private void addRawField(StringBuilder builder, String fieldValue) {
    if (0 != builder.length()) {
      builder.append(LINEBREAK);
    }
    final String escapedValue = TextUtils.htmlEncode(fieldValue);
    builder.append(String.format(RAW_FORMAT, escapedValue.replace("\n", LINEBREAK)));
  }
}
