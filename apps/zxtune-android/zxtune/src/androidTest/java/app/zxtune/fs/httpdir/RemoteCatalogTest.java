package app.zxtune.fs.httpdir;

import static org.junit.Assert.*;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.Before;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import org.junit.Test;

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

  @Test
  public void testXmlIndex() throws IOException {
    final String data = "<?xml version=\"1.0\"?>\n" +
                            "<list>\n" +
                            "<directory mtime=\"2016-09-13T22:05:29Z\">!MDScene_Arcade_VGM</directory>\n" +
                            "<directory mtime=\"2018-06-27T07:25:08Z\">x1</directory>\n" +
                            "<file mtime=\"2018-06-27T06:24:42Z\" size=\"1578655\">hoot_2018-06-26.7z</file>\n" +
                            "</list>";
    final String entries[] = {
        "!MDScene_Arcade_VGM", "",
        "x1", "",
        "hoot_2018-06-26.7z", "1.5M"
    };
    test(data, entries, Mode.CHECK_ALL);
  }

  @Test
  public void testTableIndex() throws IOException {
    final String data = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n" +
                            "<html>\n" +
                            " <head>\n" +
                            "  <title>Index of /some/page</title>\n" +
                            " </head>\n" +
                            " <body>\n" +
                            "<h1>Index of /some/page</h1>\n" +
                            "  <table>\n" +
                            "   <tr><th valign=\"top\"><img src=\"/icons/blank.gif\" alt=\"[ICO]\"></th><th><a href=\"?C=N;O=D\">Name</a></th><th><a href=\"?C=M;O=A\">Last modified</a></th><th><a href=\"?C=S;O=A\">Size</a></th><th><a href=\"?C=D;O=A\">Description</a></th></tr>\n" +
                            "   <tr><th colspan=\"5\"><hr></th></tr>\n" +
                            "<tr><td valign=\"top\"><img src=\"/icons/back.gif\" alt=\"[PARENTDIR]\"></td><td><a href=\"/ayon/\">Parent Directory</a>       </td><td>&nbsp;</td><td align=\"right\">  - </td><td>&nbsp;</td></tr>\n" +
                            "<tr><td valign=\"top\"><img src=\"/icons/folder.gif\" alt=\"[DIR]\"></td><td><a href=\"scene_cpc/\">scene_cpc/</a>             </td><td align=\"right\">2018-02-22 00:31  </td><td align=\"right\">  - </td><td>&nbsp;</td></tr>\n" +
                            "<tr><td valign=\"top\"><img src=\"/icons/unknown.gif\" alt=\"[   ]\"></td><td><a href=\"AT.ay\">AT.ay</a>                  </td><td align=\"right\">2018-04-25 03:13  </td><td align=\"right\">7.1K</td><td>&nbsp;</td></tr>\n" +
                            "   <tr><th colspan=\"5\"><hr></th></tr>\n" +
                            "</table>\n" +
                            "</body></html>\n";
    final String entries[] = {
        "scene_cpc", "",
        "AT.ay", "7.1K",
    };
    test(data, entries, Mode.CHECK_ALL);
  }

  @Test
  public void testPreIndex() throws IOException {
    final String data = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n" +
                            "<html>\n" +
                            " <head>\n" +
                            "  <title>Index of /</title>\n" +
                            " </head>\n" +
                            " <body>\n" +
                            "<h1>Index of /</h1>\n" +
                            "<pre><img src=\"/icons/blank.gif\" alt=\"Icon \"> <a href=\"?C=N;O=D\">Name</a>                    <a href=\"?C=M;O=A\">Last modified</a>      <a href=\"?C=S;O=A\">Size</a>  <a href=\"?C=D;O=A\">Description</a><hr><img src=\"/icons/folder.gif\" alt=\"[DIR]\"> <a href=\"!MDScene_Arcade_VGM/\">!MDScene_Arcade_VGM/</a>    2016-09-13 18:05    -   \n" +
                            "<img src=\"/icons/unknown.gif\" alt=\"[   ]\"> <a href=\"hoot_2018-06-26.7z\">hoot_2018-06-26.7z</a>      2018-06-27 02:24  1.5M  <span class='description'>7-Zip Archive</span>\n" +
                            "<img src=\"/icons/folder.gif\" alt=\"[DIR]\"> <a href=\"x1/\">x1/</a>                     2018-06-27 03:25    -   \n" +
                            "<hr></pre>\n" +
                            "</body></html>\n";
    final String entries[] = {
        "!MDScene_Arcade_VGM", "",
        "x1", "",
        "hoot_2018-06-26.7z", "1.5M"
    };
    test(data, entries, Mode.CHECK_ALL);
  }

  protected final void test(Path path, String[] entries) throws IOException {
    test(path, entries, Mode.CHECK_ALL);
  }

  protected final void test(Path path, String[] entries, Mode mode) throws IOException {
    final CheckingVisitor visitor = new CheckingVisitor(entries, mode);
    catalog.parseDir(path, visitor);
    visitor.check();
  }

  protected final void test(String data, String[] entries, Mode mode) throws IOException {
    final CheckingVisitor visitor = new CheckingVisitor(entries, mode);
    catalog.parseDir(ByteBuffer.wrap(data.getBytes()), visitor);
    visitor.check();
  }

  private static class CheckingVisitor implements Catalog.DirVisitor {

    private final HashMap<String, String> etalon = new HashMap<>();
    private final Mode mode;

    CheckingVisitor(String[] entries, Mode mode) {
      for (int idx = 0; idx < entries.length; idx += 2) {
        this.etalon.put(entries[idx], entries[idx + 1]);
      }
      this.mode = mode;
    }

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

    final void check() {
      if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_MISSED) {
        for (Map.Entry<String, String> nameAndSize : etalon.entrySet()) {
          fail(String.format("Missed entry '%s' %s", nameAndSize.getKey(), nameAndSize.getValue()));
        }
      }
    }
  }
}
