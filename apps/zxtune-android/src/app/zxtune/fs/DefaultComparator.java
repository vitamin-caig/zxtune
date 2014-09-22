/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs;

import java.util.Comparator;

public class DefaultComparator implements Comparator<VfsObject> {

  @Override
  public int compare(VfsObject lh, VfsObject rh) {
    return String.CASE_INSENSITIVE_ORDER.compare(lh.getName(), rh.getName());
  }
  
  public static Comparator<VfsObject> instance() {
    return Holder.INSTANCE;
  }
  
  //onDemand holder idiom
  private static class Holder {
    public static final DefaultComparator INSTANCE = new DefaultComparator();
  }
}
