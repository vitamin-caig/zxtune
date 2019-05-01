package app.zxtune.core.jni;

import static org.junit.Assert.*;

import org.junit.*;

// TODO: Not working due to lack of JNI support in instrumentation tests
@Ignore
public class JniModuleTest {

  @Test
  public void testWrongHandle() {
    try {
      new JniModule(-1);
      fail("Unreachable");
    } catch (Throwable e) {
      assertNotNull(e);
    }
  }
}