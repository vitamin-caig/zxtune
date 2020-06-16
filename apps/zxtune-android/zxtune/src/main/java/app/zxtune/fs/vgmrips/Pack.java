package app.zxtune.fs.vgmrips;

import androidx.annotation.NonNull;

// Backward links are not need
public class Pack {
  @NonNull
  public String id;
  @NonNull
  public String title;
  public int songs;
  public int score;
  public int ratings;

  Pack(String id, String title) {
    this.id = id;
    this.title = title;
  }
}
