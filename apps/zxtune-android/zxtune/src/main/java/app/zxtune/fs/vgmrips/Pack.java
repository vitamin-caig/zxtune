package app.zxtune.fs.vgmrips;

// Backward links are not need
public class Pack {
  public String id;
  public String title;
  public int songs;
  public int score;
  public int ratings;

  Pack(String id, String title) {
    this.id = id;
    this.title = title;
  }
}
