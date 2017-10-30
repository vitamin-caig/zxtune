package app.zxtune.fs.dbhelpers;

import java.io.IOException;

import app.zxtune.Analytics;

public class CommandExecutor {

  private final String id;

  public CommandExecutor(String id) {
    this.id = id;
  }

  public final void executeQuery(String scope, QueryCommand cmd) throws IOException {
    if (cmd.getLifetime().isExpired()) {
      refreshAndQuery(scope, cmd);
    } else {
      cmd.queryFromCache();
      Analytics.sendVfsCacheEvent(id, scope);
    }
  }

  private void refreshAndQuery(String scope, QueryCommand cmd) throws IOException {
    try {
      executeRefresh(scope, cmd);
      cmd.queryFromCache();
    } catch (IOException e) {
      if (cmd.queryFromCache()) {
        Analytics.sendVfsCacheEvent(id, scope);
      } else {
        throw e;
      }
    }
  }

  public final void executeRefresh(String scope, QueryCommand cmd) throws IOException {
    final Transaction transaction = cmd.startTransaction();
    try {
      cmd.updateCache();
      cmd.getLifetime().update();
      transaction.succeed();
      Analytics.sendVfsRemoteEvent(id, scope);
    } finally {
      transaction.finish();
    }
  }

  public final <T> T executeFetchCommand(String scope, FetchCommand<T> cmd) throws IOException {
    final T cached = cmd.fetchFromCache();
    if (cached != null) {
      Analytics.sendVfsCacheEvent(id, scope);
      return cached;
    }
    final T remote = cmd.fetchFromRemote();
    //log executed remote request in despite of result
    Analytics.sendVfsRemoteEvent(id, scope);
    return remote;
  }
}
