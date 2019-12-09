package app.zxtune.ui.playlist;

import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;

public class PlaylistEntry {
  public long id;
  public Identifier location;
  public String title;
  public String author;
  public TimeStamp duration;
}
