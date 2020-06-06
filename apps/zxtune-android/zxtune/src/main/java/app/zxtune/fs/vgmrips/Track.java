package app.zxtune.fs.vgmrips;

import app.zxtune.TimeStamp;

public class Track {
  public int number; //1-based
  public String title;
  public TimeStamp duration;
  public String location; //unescaped path after /packs/vgm/

  Track(int number, String title, TimeStamp duration, String location) {
    this.number = number;
    this.title = title;
    this.duration = duration;
    this.location = location;
  }
}
