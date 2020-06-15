package app.zxtune.fs.vgmrips;

import androidx.annotation.NonNull;

/**
 * Defines any grouping entity (chip, company, composer, system etc)
 */
public class Group {
  @NonNull
  public String id;
  @NonNull
  public String title;
  public int packs;

  Group(String id, String title, int packs) {
    this.id = id;
    this.title = title;
    this.packs = packs;
  }
}
