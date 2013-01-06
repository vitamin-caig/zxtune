package app.zxtune.benchmark;

public class Benchmark {

  public interface TestVisitor {
    boolean OnTest(String category, String name);
    void OnTestResult(String category, String name, double result);
  }

  static {
    System.loadLibrary("benchmark");
  }

  public static native void ForAllTests(TestVisitor visitor);
}
