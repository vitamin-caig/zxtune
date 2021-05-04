package app.zxtune.core.jni;

import static org.junit.Assert.*;

import org.junit.*;

import app.zxtune.core.PropertiesContainer;

public class JniOptionsTest {

  private Api api;

  @Before
  public void setup() {
    api = Api.instance();
  }

  @Test
  public void testStringProperties() {
    final PropertiesContainer props = api.getOptions();

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
    final PropertiesContainer props = api.getOptions();

    final String name = "integer_property";
    final long value = 123456;

    assertEquals(-1, props.getProperty(name, -1));
    assertEquals(1000, props.getProperty(name, 1000));
    props.setProperty(name, value);
    assertEquals(value, props.getProperty(name, -1));
    assertEquals(value, props.getProperty(name, 1000));
  }
}