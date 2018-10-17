package app.zxtune.fs.asma;

import android.net.Uri;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

import app.zxtune.fs.httpdir.RemoteCatalogTest;

@RunWith(AndroidJUnit4.class)
public class CatalogTest extends RemoteCatalogTest {

    @Test
    public void testRoot() throws IOException {
        final Path path = Path.create();
        //hardcoded entries, filtered in Root
        final String[] entries = {
                "Composers", "",
                "Games",     "",
                "Misc",      "",
                "Unknown",   ""
        };
        test(path, entries, Mode.CHECK_MISSED);
    }

    @Test
    public void testListing() throws IOException {
        final Path path = Path.parse(Uri.parse("asma:/Composers/Bendyk_Piotr"));
        final String[] entries = {
                "Surprise.sap",    "7.8K",
                "Surprise_2.sap",  "9.9K",
                "Third.sap",       "9.6K"
        };
        test(path, entries);
    }
}
