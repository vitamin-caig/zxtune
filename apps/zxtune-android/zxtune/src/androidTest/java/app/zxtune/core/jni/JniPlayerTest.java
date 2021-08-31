package app.zxtune.core.jni;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import org.junit.Test;

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