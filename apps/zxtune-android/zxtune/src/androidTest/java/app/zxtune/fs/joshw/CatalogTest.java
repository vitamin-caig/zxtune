package app.zxtune.fs.joshw;

import android.net.Uri;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

import app.zxtune.fs.httpdir.RemoteCatalogTest;

@RunWith(AndroidJUnit4.class)
public class CatalogTest extends RemoteCatalogTest {

    @Test
    public void testKssRoot() throws IOException {
        final Path path = Path.parse(Uri.parse("joshw:/kss"));
        final String[] entries = {
                "Game Gear",        "",
                "MSX",              "",
                "Master System",    ""
        };
        test(path, entries);
    }

    @Test
    public void testNsfN() throws IOException {
        final Path path = Path.parse(Uri.parse("joshw:/nsf/n"));
        final String[] entries = {
                //first
                "NARC (1990-08)(Williams)(Rare)(Acclaim).7z",   "3.3K",
                //escaped
                "North & South (1990-09-21)(Kemco).7z",         "11K",
                //last
                "Nuts & Milk (1984-07-28)(Hudson).7z",          "3.0K"
        };
        test(path, entries, Mode.CHECK_MISSED);
    }
}
