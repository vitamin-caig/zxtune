package app.zxtune.core.jni;

import static org.junit.Assert.*;

import org.junit.*;

public class JniPlayerTest {

  @Test
  public void testWrongHandle() {
    try {
      new JniPlayer(-1);
      fail("Unreachable");
    } catch (Throwable e) {
      assertNotNull(e);
    }
  }
}