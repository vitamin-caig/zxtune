package app.zxtune.ui.playlist;

import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;

// public for binding
public class Entry {
  public long id;
  public Identifier location;
  public String title;
  public String author;
  public TimeStamp duration;
}
