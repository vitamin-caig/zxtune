package app.zxtune.fs.hvsc;

import android.net.Uri;

import androidx.test.ext.junit.runners.AndroidJUnit4;

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
                "DEMOS",        "",
                "GAMES",        "",
                "MUSICIANS",    ""
        };
        test(path, entries, Mode.CHECK_MISSED);
    }

    @Test
    public void testListing() throws IOException {
        final Path path = Path.parse(Uri.parse("hvsc:/MUSICIANS/O/OPM"));
        final String[] entries = {
                "Fantjes_Beat.sid",             "1.9K",
                "Rockmonitor_3.sid",            "35.1K",//35K for native
                "Rockmonitor_5_Demosong.sid",   "18.3K",//18K for native
                "Sid_Slam.sid",                 "1.8K",
                "Zoolook.sid",                  "3.2K"
        };
        test(path, entries);
    }
}
