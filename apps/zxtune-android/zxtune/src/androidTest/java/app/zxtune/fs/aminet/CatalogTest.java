package app.zxtune.fs.aminet;

import android.content.Context;
import android.net.Uri;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;

import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.httpdir.RemoteCatalogTest;

@RunWith(AndroidJUnit4.class)
public class CatalogTest extends RemoteCatalogTest {

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final HttpProvider http = HttpProviderFactory.createProvider(ctx);
    catalog = new RemoteCatalog(new MultisourceHttpProvider(http));
  }

  @Test
  public void testRoot() throws IOException {
    final Path path = Path.create();
    //hardcoded entries, filtered in Root
    final String[] entries = {
        "32bit", "@MP3s by 32bit choonz (2 packages)",
        "xm", "@Multichannel xm mods (413 packages)"
    };
    test(path, entries, Mode.CHECK_MISSED);
  }

  @Test
  public void testListing() throws IOException {
    final Path path = Path.parse(Uri.parse("aminet:/piano"));
    final String[] entries = {
        "/piano/BananaRag.lha", "33K@Banana Rag. Cool rag tune by HeLMeR",
        //last one
        "/piano/wzl_laudamus.lzh", "99K@Protracker module by The Weasel ****+"
    };
    test(path, entries, Mode.CHECK_MISSED);
  }

  @Test
  public void testSearch() throws IOException {
    final String[] entries = {
        //filename
        "/misc/BeginLife.lha", "116K@Rymix/[Death] mod: 'Beginning of Life'",
        //description
        "/neuro/nd-cls03.lha", "105K@The new Beginning (older Module by Neurodancer/1oo%)"
    };
    final CheckingVisitor visitor = new CheckingVisitor(entries, Mode.CHECK_MISSED);
    ((RemoteCatalog) catalog).find("begin", visitor);
    visitor.check();
  }
}
