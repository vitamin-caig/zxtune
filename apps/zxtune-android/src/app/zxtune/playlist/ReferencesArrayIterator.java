/**
 *
 * @file
 *
 * @brief  Array-based implementation of ReferencesIterator
 *
 * @author vitamin.caig@gmail.com
 *
 */
package app.zxtune.playlist;

import java.util.ArrayList;

class ReferencesArrayIterator implements ReferencesIterator {
  
  private final ArrayList<Entry> entries;
  private int current;
  
  public ReferencesArrayIterator(ArrayList<Entry> entries) {
    this.entries = entries;
    this.current = 0;
  }
  
  @Override
  public Entry getItem() {
    return entries.get(current);
  }
  
  @Override
  public boolean next() {
    if (current >= entries.size() - 1) {
      return false;
    } else {
      ++current;
      return true;
    }
  }
  
  @Override
  public final boolean prev() {
    if (0 == current) {
      return false;
    } else {
      --current;
      return true;
    }
  }
}
