package app.zxtune.core.jni;

import static org.junit.Assert.*;

import org.junit.*;

public class GlobalOptionsTest {

  @Test
  public void testStringProperties() {
    final GlobalOptions props = GlobalOptions.instance();

    final String name = "string_property";
    final String value = "string value";

    assertEquals("", props.getProperty(name, ""));
    assertEquals("defval", props.getProperty(name, "defval"));
    props.setProperty(name, value);
    assertEquals(value, props.getProperty(name, ""));
    assertEquals(value, props.getProperty(name, "defval"));
  }

  @Test
  public void testIntegerProperties() {
    final GlobalOptions props = GlobalOptions.instance();

    final String name = "integer_property";
    final long value = 123456;

    assertEquals(-1, props.getProperty(name, -1));
    assertEquals(1000, props.getProperty(name, 1000));
    props.setProperty(name, value);
    assertEquals(value, props.getProperty(name, -1));
    assertEquals(value, props.getProperty(name, 1000));
  }
}