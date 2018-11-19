package app.zxtune.fs.http;

import static org.junit.Assert.*;

import android.content.Context;
import android.net.Uri;
import android.support.test.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

public class ProviderTest {

    protected HttpProvider provider;

    @Before
    public void setUp() {
        final Context ctx = InstrumentationRegistry.getTargetContext();
        provider = HttpProviderFactory.createProvider(ctx);
    }

    @Test
    public void testObjectStatic() throws IOException {
        final Uri uri = Uri.parse("http://nsf.joshw.info/n/North%20%26%20South%20(1990-09-21)(Kemco).7z");
        final HttpObject obj = provider.getObject(uri);
        assertEquals(uri, obj.getUri());
        assertEquals(10855, obj.getContentLength().longValue());
        //$ date -u -d 'Sun, 14 Jul 2013 17:32:52 GMT' +%s
        //1373823172
        assertEquals(1373823172, obj.getLastModified().convertTo(TimeUnit.SECONDS));
    }

    @Test
    public void testObjectRedirect() throws IOException {
        final HttpObject obj = provider.getObject(Uri.parse("http://amp.dascene.net/downmod.php?index=150946"));
        assertEquals("http://amp.dascene.net/modules/E/Estrayk/MOD.%28Starquake%29broken%20heart.gz", obj.getUri().toString());
        assertEquals(182985, obj.getContentLength().longValue());
        //$ date -u -d 'Sat, 17 Nov 2018 23:04:27 GMT' +%s
        //1542495867
        assertEquals(1542495867, obj.getLastModified().convertTo(TimeUnit.SECONDS));
    }

    @Test
    public void testObjectDynamic() throws IOException {
        final Uri uri = Uri.parse("https://storage.zxtune.ru/check");
        final HttpObject obj = provider.getObject(uri);
        assertEquals(uri, obj.getUri());
        assertNull(obj.getContentLength());
        assertNull(obj.getLastModified());
    }

    @Test
    public void testUnavailableHost() {
        try {
            final HttpObject obj = provider.getObject(Uri.parse("http://invalid.zxtune.ru/document"));
            assertTrue("Should not create object", false);
        } catch (IOException e) {
            assertNotNull("Thrown exception", e);
        }
    }

    @Test
    public void testUnavailableFile() {
        try {
            final HttpObject obj = provider.getObject(Uri.parse("http://nsf.joshw.info/ne/"));
            assertTrue("Should not create object", false);
        } catch (IOException e) {
            assertNotNull("Thrown exception", e);
            assertEquals("Not Found", e.getMessage());
        }
    }
}
