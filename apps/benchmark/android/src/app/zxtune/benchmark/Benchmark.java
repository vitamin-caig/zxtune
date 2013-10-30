/**
 *
 * @file
 *
 * @brief Gate to jni code
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.benchmark;

public class Benchmark {

  public interface TestVisitor {
    boolean OnTest(String category, String name);
    void OnTestResult(String category, String name, double result);
  }

  static {
    System.loadLibrary("benchmark");
  }

  public static native synchronized void ForAllTests(TestVisitor visitor);
}
