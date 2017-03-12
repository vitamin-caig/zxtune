package app.zxtune.fs.dbhelpers;

import java.io.IOException;

import app.zxtune.Analytics;

public class CommandExecutor {

  private final String id;

  public CommandExecutor(String id) {
    this.id = id;
  }

  public final void executeQueryCommand(String scope, QueryCommand cmd) throws IOException {
    final Timestamps.Lifetime lifetime = cmd.getLifetime();
    IOException remoteError = null;
    if (lifetime.isExpired()) {
      final Transaction transaction = cmd.startTransaction();
      try {
        cmd.queryFromRemote();
        lifetime.update();
        transaction.succeed();
        Analytics.sendVfsRemoteEvent(id, scope);
        return;
      } catch (IOException e) {
        remoteError = e;
      } finally {
        transaction.finish();
      }
    }
    if (!cmd.queryFromCache() && remoteError != null) {
      throw remoteError;
    }
    Analytics.sendVfsCacheEvent(id, scope);
  }

  public final <T> T executeFetchCommand(String scope, FetchCommand<T> cmd) throws IOException {
    final T cached = cmd.fetchFromCache();
    if (cached != null) {
      Analytics.sendVfsCacheEvent(id, scope);
      return cached;
    }
    final T remote = cmd.fetchFromRemote();
    if (remote != null) {
      Analytics.sendVfsRemoteEvent(id, scope);
    }
    return remote;
  }
}
