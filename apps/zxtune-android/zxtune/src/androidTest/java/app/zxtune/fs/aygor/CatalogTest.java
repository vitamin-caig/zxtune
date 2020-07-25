package app.zxtune.fs.aygor;

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
        final String[] entries = {
                "games",        "",
                "musicians",    "",
                "scene",        ""
        };
        test(path, entries);
    }

    @Test
    public void testTruncate() throws IOException {
        final Path path = Path.parse(Uri.parse("aygor:/musicians/A/AS"));
        final String[] entries = {
                "Prince_of_Persia_intro.ay",    "16K"
        };
        test(path, entries);
    }

    @Test
    public void testSuspiciousFiles() throws IOException {
        final Path path = Path.parse(Uri.parse("aygor:/scene/scene_cpc"));
        final String[] entries = {
                //not an .ay
                "!info.txt",                "36",
                //dir
                "Audio_Illusions",          "",
                //truncated
                "CPC_Telegramm_16[CPC].ay", "8.6K",
                "CPC_Telegramm_17[CPC].ay", "9.5K",
        };
        test(path, entries, Mode.CHECK_MISSED);
    }
}
