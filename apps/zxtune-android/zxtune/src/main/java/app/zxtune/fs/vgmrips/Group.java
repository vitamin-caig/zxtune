package app.zxtune.fs.vgmrips;

/**
 * Defines any grouping entity (chip, company, composer, system etc)
 */
public class Group {
  public String id;
  public String title;
  public int packs;

  Group(String id, String title, int packs) {
    this.id = id;
    this.title = title;
    this.packs = packs;
  }
}
