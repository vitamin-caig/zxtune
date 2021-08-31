package app.zxtune.io;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class TransactionalOutputStreamTest {

  private static File getFileName(String name) {
    return new File(System.getProperty("java.io.tmpdir", "."), name);
  }

  private static File getNonexistingFilename(String name) {
    final File res = getFileName(name);
    res.delete();
    assertFalse(res.exists());
    return res;
  }

  private static File getExistingFilename(String name) {
    final File res = getFileName(name);
    assertTrue(res.isFile());
    return res;
  }

  private static void fill(int fill, int size, OutputStream out) throws IOException {
    for (int i = 0; i < size; ++i) {
      out.write(fill);
    }
  }

  private static void testFile(int fill, int size, File file) throws IOException {
    assertTrue(file.isFile());
    assertEquals(size, file.length());
    final InputStream in = new FileInputStream(file);
    try {
      test(fill, size, in);
    } finally {
      in.close();
    }
  }

  private static void test(int fill, int size, InputStream in) throws IOException {
    for (int i = 0; i < size; ++i) {
      assertEquals(fill, in.read());
    }
    assertEquals(-1, in.read());
  }

  @Test
  public void testNewEmpty() throws IOException {
    final File file = getNonexistingFilename("tos1");
    {
      final OutputStream stream = new TransactionalOutputStream(file);
      stream.flush();
      stream.close();
    }
    testFile(0, 0, file);
  }

  @Test
  public void testNewNonEmpty() throws IOException {
    final File file = getNonexistingFilename("tos2");
    {
      final OutputStream stream = new TransactionalOutputStream(file);
      fill(1, 100, stream);
      stream.flush();
      stream.close();
    }
    testFile(1, 100, file);
  }

  @Test
  public void testNonConfirmed() throws IOException {
    final File file = getNonexistingFilename("tos3");
    {
      final OutputStream stream = new TransactionalOutputStream(file);
      fill(2, 100, stream);
      stream.close();
    }
    assertFalse(file.exists());
  }

  @Test
  public void testOverwrite() throws IOException {
    testNewNonEmpty();
    final File file = getExistingFilename("tos2");
    testFile(1, 100, file);
    {
      final OutputStream stream = new TransactionalOutputStream(file);
      fill(3, 300, stream);
      stream.flush();
      stream.close();
    }
    testFile(3, 300, file);
  }

  @Test
  public void testOverwriteNonConfirmed() throws IOException {
    testNewNonEmpty();
    final File file = getExistingFilename("tos2");
    testFile(1, 100, file);
    {
      final OutputStream stream = new TransactionalOutputStream(file);
      fill(3, 300, stream);
      stream.close();
    }
    testFile(1, 100, file);
  }

  /*
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
      assertTrue("Unreachable", false);
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
    try {
      //really FileInputStream ctor throws
      Io.readFrom(new FileInputStream(file));
      assertTrue("Unreachable", false);
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
  }

  @Test
  public void testEmpty() throws IOException {
    final File file = createFile(0, 0);
    try {
      Io.readFrom(file);
      assertTrue("Unreachable", false);
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
    final InputStream stream = new FileInputStream(file);
    try {
      Io.readFrom(stream);
      assertTrue("Unreachable", false);
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
      assertFalse(buf instanceof MappedByteBuffer);
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
      assertTrue("Unreachable", false);
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
    try {
      Io.readFrom(new FileInputStream(file), 3 * 1024);
      assertTrue("Unreachable", false);
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
  }
  */
}
