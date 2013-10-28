/**
 * @file
 * @brief Releaseable interface
 * @version $Id:$
 * @author Vitamin/CAIG
 */
package app.zxtune;

/**
 * Base interface for managed resources supporting release call
 * Analogue of Closeable but do not throw any specific exceptions
 */
public interface Releaseable {
  
  public void release();
}
