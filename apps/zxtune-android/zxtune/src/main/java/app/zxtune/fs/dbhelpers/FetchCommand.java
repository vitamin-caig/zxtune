package app.zxtune.fs.dbhelpers;

import java.io.IOException;
import java.nio.ByteBuffer;

public interface FetchCommand {
  ByteBuffer fetchFromCache();
  ByteBuffer fetchFromRemote() throws IOException;
}
