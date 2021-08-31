package app.zxtune.fs.http;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

import android.content.Context;
import android.net.Uri;
import android.os.Build;

import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.TimeUnit;

import app.zxtune.BuildConfig;

public class ProviderTest {

  protected HttpProvider provider;

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    provider = HttpProviderFactory.createProvider(ctx);
  }

  private static void testStream(InputStream stream, long size, byte[] head, byte[] tail) throws IOException {
    try {
      final byte[] realHead = new byte[head.length];
      assertEquals(head.length, stream.read(realHead));
      assertArrayEquals(head, realHead);
      for (long toSkip = size - head.length - tail.length; toSkip != 0; ) {
        toSkip -= stream.skip(toSkip);
      }
      final byte[] realTail = new byte[tail.length];
      assertEquals(tail.length, stream.read(realTail));
      assertArrayEquals(tail, realTail);
      assertEquals(-1, stream.read());
    } finally {
      stream.close();
    }
  }

  @Test
  public void testObjectStatic() throws IOException {
    final Uri uri = Uri.parse("http://nsf.joshw.info/n/North%20%26%20South%20(1990-09-21)(Kemco)[NES].7z");
    final HttpObject obj = provider.getObject(uri);
    assertEquals(uri, obj.getUri());
    assertEquals(10855, obj.getContentLength().longValue());
    //$ date -u -d 'Sun, 14 Jul 2013 17:32:52 GMT' +%s
    //1373823172
    assertEquals(1373823172, obj.getLastModified().convertTo(TimeUnit.SECONDS));
    testStream(obj.getInput(), 10855, new byte[]{0x37, 0x7a, (byte) 0xbc, (byte) 0xaf, 0x27, 0x1c, 0x00, 0x03}, new byte[]{0x30, 0x2e, 0x39, 0x62, 0x65, 0x74, 0x61});
  }

  @Test
  public void testObjectRedirect() throws IOException {
    final HttpObject obj = provider.getObject(Uri.parse("http://amp.dascene.net/downmod.php?index=150946"));
    assertEquals("http://amp.dascene.net/modules/E/Estrayk/MOD.%28Starquake%29broken%20heart.gz", obj.getUri().toString());
    assertEquals(182985, obj.getContentLength().longValue());
    //$ date -u -d 'Sat, 17 Nov 2018 23:04:27 GMT' +%s
    //1542495867
    assertEquals(1542495867, obj.getLastModified().convertTo(TimeUnit.SECONDS));
    testStream(obj.getInput(), 182985, new byte[]{0x1f, (byte) 0x8b, 0x08, 0x08, 0x7b, (byte) 0x9e, (byte) 0xf0, 0x5b},
        new byte[]{0x03, (byte) 0xa7, (byte) 0x9f, 0x12, (byte) 0x99, 0x24, (byte) 0xb3, 0x04, 0x00});
  }

  @Test
  public void testObjectDynamic() throws IOException {
    final Uri uri = Uri.parse("https://storage.zxtune.ru/check");
    final HttpObject obj = provider.getObject(uri);
    assertEquals(uri, obj.getUri());
    assertNull(obj.getContentLength());
    assertNull(obj.getLastModified());
    testStream(obj.getInput(), 0x3a, new byte[]{0x7b, 0x22, 0x69, 0x70, 0x22, 0x3a, 0x22}, new byte[]{0x22, 0x7d, 0x0a});
  }

  @Test
  public void testUnavailableHost() {
    try {
      final HttpObject obj = provider.getObject(Uri.parse("http://invalid.zxtune.ru/document"));
      fail("Should not create object");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
    }
  }

  @Test
  public void testUnavailableFile() {
    try {
      final HttpObject obj = provider.getObject(Uri.parse("http://nsf.joshw.info/ne/"));
      fail("Should not create object");
    } catch (IOException e) {
      assertNotNull("Thrown exception", e);
      assertEquals("Unexpected code 404 (Not Found) for HEAD request to http://nsf.joshw.info/ne/", e.getMessage());
    }
  }

  @Test
  public void testBigFile() throws IOException {
    final Uri uri = Uri.parse(BuildConfig.CDN_ROOT + "/browse/joshw/pc/t/Tekken 7 (2017-06-02)(-)(Bandai Namco)[PC].7z");
    final HttpObject obj = provider.getObject(uri);
    assertEquals(uri, obj.getUri());
    if (Build.VERSION.SDK_INT >= 24) {
      assertEquals(3683414322L, obj.getContentLength().longValue());
    } else {
      assertNull(obj.getContentLength());
    }
        /*
        testStream(obj.getInput(), 3683414322l, new byte[] {0x37, 0x7a, (byte) 0xbc, (byte) 0xaf, 0x27, 0x1c, 0x00, 0x04},
                new byte[] {0x0a, 0x01, (byte) 0xfd, (byte) 0x94, (byte) 0xa6, 0x6e, 0x00, 0x00});
        */
  }
}
