/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import java.io.IOException;

public interface QueryCommand {

  boolean isCacheExpired();

  void updateCache() throws IOException;

  boolean queryFromCache();
}
