package app.zxtune.fs;

import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.Nullable;

import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;

import java.io.IOException;
import java.io.InputStream;

public class HtmlUtils {

  public static Document parseDoc(InputStream input) throws IOException {
    try {
      return Jsoup.parse(input, null, "");
    } finally {
      input.close();
    }
  }

  @Nullable
  public static Integer getQueryInt(Element anchor, String param) {
    final String raw = Uri.parse(anchor.attr("href")).getQueryParameter(param);
    return tryGetInteger(raw);
  }

  @Nullable
  public static Integer tryGetInteger(@Nullable String raw) {
    if (TextUtils.isEmpty(raw)) {
      return null;
    }
    final String txt = raw.trim();
    return TextUtils.isEmpty(txt) || !TextUtils.isDigitsOnly(txt) ? null : Integer.valueOf(txt);
  }
}
