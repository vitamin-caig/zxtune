/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import java.io.IOException;

public final class VfsExtensions {

  // Remote autoplay URI, string
  public static final String SHARE_URL = "SHARE_URL";

  // Separate interface for fast searching
  public interface SearchEngine {

    interface Visitor {
      void onFile(VfsFile file);
    }

    void find(String query, Visitor visitor) throws IOException;
  }

  public static final String SEARCH_ENGINE = "SEARCH_ENGINE";

  // Comparator<VfsObject>
  public static final String COMPARATOR = "COMPARATOR";

  // java.util.Iterator<VfsFile>
  public static final String FEED = "FEED";

  // File
  public static final String CACHE = "CACHE";

  // File
  public static final String FILE = "FILE";
}
