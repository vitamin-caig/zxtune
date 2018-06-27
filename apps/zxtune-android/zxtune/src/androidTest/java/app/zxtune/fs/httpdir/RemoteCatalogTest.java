package app.zxtune.fs.httpdir;

import static org.junit.Assert.*;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.Before;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;

public class RemoteCatalogTest {

    protected RemoteCatalog catalog;

    protected enum Mode {
        CHECK_EXISTING,
        CHECK_MISSED,
        CHECK_ALL
    }

    @Before
    public void setUp() {
        final Context ctx = InstrumentationRegistry.getTargetContext();
        final HttpProvider http = HttpProviderFactory.createProvider(ctx);
        catalog = new RemoteCatalog(http);
    }

    protected final void test(Path path, String[] entries) throws IOException {
        test(path, entries, Mode.CHECK_ALL);
    }

    protected final void test(Path path, String[] entries, final Mode mode) throws IOException {
        final HashMap<String, String> etalon = new HashMap<>(entries.length / 2);
        for (int idx = 0; idx < entries.length; idx += 2) {
            etalon.put(entries[idx], entries[idx + 1]);
        }
        catalog.parseDir(path, new Catalog.DirVisitor() {
            @Override
            public void acceptDir(String name) {
                test(name, "");
            }

            @Override
            public void acceptFile(String name, String size) {
                test(name, size);
            }

            private void test(String name, String size) {
                final String expectedSize = etalon.remove(name);
                if (expectedSize != null) {
                    assertEquals("Invalid size", expectedSize, size);
                } else if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_EXISTING) {
                    assertNotNull(String.format("Unexpected entry '%s' %s", name, size), expectedSize);
                }
            }
        });
        if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_MISSED) {
            for (Map.Entry<String, String> nameAndSize : etalon.entrySet()) {
                fail(String.format("Missed entry '%s' %s", nameAndSize.getKey(), nameAndSize.getValue()));
            }
        }
    }
}
