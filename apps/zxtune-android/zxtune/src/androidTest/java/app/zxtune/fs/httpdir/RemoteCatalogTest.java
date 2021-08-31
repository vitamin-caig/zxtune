package app.zxtune.fs.httpdir;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import android.content.Context;

import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;

public class RemoteCatalogTest {

  protected RemoteCatalog catalog;

  protected enum Mode {
    CHECK_EXISTING,
    CHECK_MISSED,
    CHECK_ALL
  }

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final HttpProvider http = HttpProviderFactory.createProvider(ctx);
    catalog = new RemoteCatalog(new MultisourceHttpProvider(http));
  }

  @Test
  public void testXmlIndex() throws IOException {
    final String data = "<?xml version=\"1.0\"?>\n" +
                            "<list>\n" +
                            "<directory mtime=\"2016-09-13T22:05:29Z\">!MDScene_Arcade_VGM</directory>\n" +
                            "<directory mtime=\"2018-06-27T07:25:08Z\">x1</directory>\n" +
                            "<file mtime=\"2018-06-27T06:24:42Z\" size=\"1578655\">hoot_2018-06-26.7z</file>\n" +
                            "</list>";
    final String[] entries = {
        "!MDScene_Arcade_VGM", "@2016-09-13T22:05:29Z",
        "x1", "@2018-06-27T07:25:08Z",
        "hoot_2018-06-26.7z", "1.5M@2018-06-27T06:24:42Z"
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
    final String[] entries = {
        "scene_cpc", "@2018-02-22 00:31",
        "AT.ay", "7.1K@2018-04-25 03:13",
    };
    test(data, entries, Mode.CHECK_ALL);
  }

  // scene.org
  @Test
  public void testTableIndex2() throws IOException {
    final String data = "<!DOCTYPE html>\n" +
        "<html>\n" +
        "<head>\n" +
        "<meta charset=\"utf-8\">\n" +
        "<title>Index of /pub/music/groups/2063music/</title>\n" +
        "<style type=\"text/css\">\n" +
        "a, a:active {text-decoration: none; color: blue;}\n" +
        "a:visited {color: #48468F;}\n" +
        "a:hover, a:focus {text-decoration: underline; color: red;}\n" +
        "body {background-color: #F5F5F5;}\n" +
        "h2 {margin-bottom: 12px;}\n" +
        "table {margin-left: 12px;}\n" +
        "th, td { font: 90% monospace; text-align: left;}\n" +
        "th { font-weight: bold; padding-right: 14px; padding-bottom: 3px;}\n" +
        "td {padding-right: 14px;}\n" +
        "td.s, th.s {text-align: right;}\n" +
        "div.list { background-color: white; border-top: 1px solid #646464; border-bottom: 1px solid #646464; padding-top: 10px; padding-bottom: 14px;}\n" +
        "div.foot { font: 90% monospace; color: #787878; padding-top: 4px;}\n" +
        "</style>\n" +
        "</head>\n" +
        "<body>\n" +
        "<h2>Index of /pub/music/groups/2063music/</h2>\n" +
        "<div class=\"list\">\n" +
        "<table summary=\"Directory Listing\" cellpadding=\"0\" cellspacing=\"0\">\n" +
        "<thead><tr><th class=\"n\">Name</th><th class=\"m\">Last Modified</th><th class=\"s\">Size</th><th class=\"t\">Type</th></tr></thead>\n" +
        "<tbody>\n" +
        "<tr class=\"d\"><td class=\"n\"><a href=\"../\">..</a>/</td><td class=\"m\">&nbsp;</td><td class=\"s\">- &nbsp;</td><td class=\"t\">Directory</td></tr>\n" +
        "<tr class=\"d\"><td class=\"n\"><a href=\"020200/\">020200</a>/</td><td class=\"m\">2002-Jul-10 19:36:52</td><td class=\"s\">- &nbsp;</td><td class=\"t\">Directory</td></tr>\n" +
        "<tr class=\"d\"><td class=\"n\"><a href=\"activities/\">activities</a>/</td><td class=\"m\">2004-Jun-10 13:03:57</td><td class=\"s\">- &nbsp;</td><td class=\"t\">Directory</td></tr>\n" +
        "<tr><td class=\"n\"><a href=\"2063music.txt\">2063music.txt</a></td><td class=\"m\">2004-Jun-09 14:19:52</td><td class=\"s\">0.6K</td><td class=\"t\">text/plain</td></tr>\n" +
        "<tr><td class=\"n\"><a href=\"63_001-opal2000-gatev0-5.mp3\">63_001-opal2000-gatev0-5.mp3</a></td><td class=\"m\">2000-Jun-22 03:23:54</td><td class=\"s\">5.1M</td><td class=\"t\">audio/mpeg</td></tr>\n" +
        "<tr><td class=\"n\"><a href=\"63_002-opal2000-home_office" +
        ".mp3\">63_002-opal2000-home_office.mp3</a></td><td class=\"m\">2000-Nov-10 " +
        "01:38:00</td><td class=\"s\">4.8M</td><td class=\"t\">audio/mpeg</td></tr>" +
        "</tbody>\n" +
        "</table>\n" +
        "</div></body>\n" +
        "</html>"
        ;
    final String[] entries = {
        "020200", "@2002-Jul-10 19:36:52",
        "activities", "@2004-Jun-10 13:03:57",
        "2063music.txt", "0.6K@2004-Jun-09 14:19:52",
        "63_001-opal2000-gatev0-5.mp3", "5.1M@2000-Jun-22 03:23:54",
        "63_002-opal2000-home_office.mp3", "4.8M@2000-Nov-10 01:38:00"
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
    final String[] entries = {
        "!MDScene_Arcade_VGM", "@2016-09-13 18:05",
        "x1", "@2018-06-27 03:25",
        "hoot_2018-06-26.7z", "1.5M@2018-06-27 02:24"
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
    catalog.parseDir(new ByteArrayInputStream(data.getBytes()), visitor);
    visitor.check();
  }

  public static class CheckingVisitor implements Catalog.DirVisitor {

    private final HashMap<String, String> etalon = new HashMap<>();
    private final Mode mode;

    public CheckingVisitor(String[] entries, Mode mode) {
      for (int idx = 0; idx < entries.length; idx += 2) {
        this.etalon.put(entries[idx], entries[idx + 1]);
      }
      this.mode = mode;
    }

    @Override
    public void acceptDir(String name, String descr) {
      acceptFile(name, descr, "");
    }

    @Override
    public void acceptFile(String name, String descr, String size) {
      test(name, size, descr);
    }

    private void test(String name, String size, String descr) {
      final String expected = etalon.remove(name);
      if (expected != null) {
        final String[] expectedSizeAndDescr = expected.split("@");
        if (expectedSizeAndDescr.length == 2) {
          assertEquals("Invalid size", expectedSizeAndDescr[0], size);
          assertEquals("Invalid description", expectedSizeAndDescr[1], descr);
        } else {
          assertEquals("Invalid size", expected, size);
        }
      } else if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_EXISTING) {
        assertNotNull(String.format("Unexpected entry '%s' %s", name, size), expected);
      }
    }

    public final void check() {
      if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_MISSED) {
        for (Map.Entry<String, String> nameAndSize : etalon.entrySet()) {
          fail(String.format("Missed entry '%s' %s", nameAndSize.getKey(), nameAndSize.getValue()));
        }
      }
    }
  }
}
