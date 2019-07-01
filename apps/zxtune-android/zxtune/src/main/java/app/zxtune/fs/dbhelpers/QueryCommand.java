/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import java.io.IOException;

public interface QueryCommand {

  String getScope();// for analytics

  Timestamps.Lifetime getLifetime();

  Transaction startTransaction() throws IOException;

  void updateCache() throws IOException;

  boolean queryFromCache();
}
