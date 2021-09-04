package app.zxtune.io;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;

public class IoTest {

  private static void generate(int fill, int size, File obj) throws IOException {
    final FileOutputStream out = new FileOutputStream(obj);
    for (int off = 0; off < size; ++off) {
      out.write(fill);
    }
    out.close();
  }

  private static File createFile(int fill, int size) throws IOException {
    final File result = File.createTempFile("test", "io");
    if (size != 0) {
      generate(fill, size, result);
    }
    assertTrue(result.canRead());
    assertEquals(size, result.length());
    return result;
  }

  private static void checkBuffer(ByteBuffer buf, int fill, int size) {
    assertEquals(size, buf.limit());
    for (int idx = 0; idx < size; ++idx) {
      assertEquals(fill, buf.get(idx));
    }
  }

  @Test
  public void testNonexisting() {
    final File file = new File("nonexisting_name");
    assertFalse(file.exists());
    try {
      Io.readFrom(file);
      fail("Unreachable");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
    try {
      //really FileInputStream ctor throws
      Io.readFrom(new FileInputStream(file));
      fail("Unreachable");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
  }

  @Test
  public void testEmpty() throws IOException {
    final File file = createFile(0, 0);
    try {
      Io.readFrom(file);
      fail("Unreachable");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
    final InputStream stream = new FileInputStream(file);
    try {
      Io.readFrom(stream);
      fail("Unreachable");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
  }

  @Test
  public void test1k() throws IOException {
    final int size = 1024;
    assertTrue(size < Io.INITIAL_BUFFER_SIZE);
    assertTrue(size < Io.MIN_MMAPED_FILE_SIZE);
    final File file = createFile(1, size);
    {
      final ByteBuffer buf = Io.readFrom(file);
      assertTrue(buf.isDirect());
      // Seems like for some versions DirectBuffer is mapped really
      //assertFalse(buf instanceof MappedByteBuffer);
      checkBuffer(buf, 1, size);
    }
    {
      final ByteBuffer buf = Io.readFrom(new FileInputStream(file));
      assertFalse(buf.isDirect());
      checkBuffer(buf, 1, size);
    }
    {
      final ByteBuffer buf = Io.readFrom(new FileInputStream(file), size);
      assertTrue(buf.isDirect());
      checkBuffer(buf, 1, size);
    }
  }

  @Test
  public void test300k() throws IOException {
    final int size = 300 * 1024;
    assertTrue(size > Io.INITIAL_BUFFER_SIZE);
    assertTrue(size > Io.MIN_MMAPED_FILE_SIZE);
    final File file = createFile(2, size);
    {
      final ByteBuffer buf = Io.readFrom(file);
      assertTrue(buf.isDirect());
      assertTrue(buf instanceof MappedByteBuffer);
      checkBuffer(buf, 2, size);
    }
    {
      final ByteBuffer buf = Io.readFrom(new FileInputStream(file));
      assertFalse(buf.isDirect());
      checkBuffer(buf, 2, size);
    }
    {
      final ByteBuffer buf = Io.readFrom(new FileInputStream(file), size);
      assertTrue(buf.isDirect());
      checkBuffer(buf, 2, size);
    }
  }

  @Test
  public void testWrongSizeHint() throws IOException {
    final File file = createFile(3, 2 * 1024);
    checkBuffer(Io.readFrom(new FileInputStream(file)), 3, 2 * 1024);
    try {
      Io.readFrom(new FileInputStream(file), 1024);
      fail("Unreachable");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
    try {
      Io.readFrom(new FileInputStream(file), 3 * 1024);
      fail("Unreachable");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
  }
}
