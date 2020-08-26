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
import java.util.Random;

public class ReferencesArrayIterator implements ReferencesIterator {
  
  private final ArrayList<Entry> entries;
  private int current;
  
  public ReferencesArrayIterator(ArrayList<Entry> entries) {
    this.entries = entries;
    this.current = -1;
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
  public boolean prev() {
    if (0 == current) {
      return false;
    } else {
      --current;
      return true;
    }
  }

  @Override
  public boolean first() {
    if (entries.isEmpty()) {
      return false;
    } else {
      current = 0;
      return true;
    }
  }
  
  @Override
  public boolean last() {
    final int size = entries.size();
    if (0 == size) {
      return false;
    } else {
      current = size - 1;
      return true;
    }
  }
  
  @Override
  public boolean random() {
    final int size = entries.size();
    if (0 == size) {
      return false;
    } else {
      current = new Random().nextInt(size);
      return true;
    }
  }
}
