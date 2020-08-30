package app.zxtune.playlist.xspf;

import android.net.Uri;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.core.Identifier;
import app.zxtune.io.Io;
import app.zxtune.playlist.Item;
import app.zxtune.playlist.ReferencesIterator;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * TODO:
 * - item's properties
 */

@RunWith(AndroidJUnit4.class)
public class XspfPlaylistTest {

  private static InputStream getPlaylistResource(String name) {
    return XspfPlaylistTest.class.getClassLoader().getResourceAsStream("playlists/" + name +
        ".xspf");
  }

  private static String getPlaylistReference(String name) throws IOException {
    final ByteArrayOutputStream out = new ByteArrayOutputStream();
    Io.copy(getPlaylistResource(name), out);
    return out.toString();
  }

  @Test(expected = IOException.class)
  public void parseEmptyString() throws IOException {
    XspfIterator.parse(getPlaylistResource("zero"));
    fail("Should not be called");
  }

  @Test
  public void parseEmpty() throws IOException {
    final ArrayList<ReferencesIterator.Entry> list = XspfIterator.parse(getPlaylistResource("empty"));
    assertTrue(list.isEmpty());
  }

  @Test
  public void parseMobileV1() throws IOException {
    final ArrayList<ReferencesIterator.Entry> list = XspfIterator.parse(getPlaylistResource(
        "mobile_v1"));
    assertEquals(3, list.size());
    {
      final ReferencesIterator.Entry entry = list.get(0);
      assertEquals("file:/folder/only%20duration", entry.location);
      assertEquals(TimeStamp.createFrom(123456, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("", entry.title);
      assertEquals("", entry.author);
    }
    {
      final ReferencesIterator.Entry entry = list.get(1);
      assertEquals("joshw:/some/%D1%84%D0%B0%D0%B9%D0%BB.7z#%D0%BF%D0%BE%D0%B4%D0%BF%D0%B0%D0%BF%D0%BA%D0%B0%2Fsubfile.mp3", entry.location);
      assertEquals(TimeStamp.createFrom(2345, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Название", entry.title);
      assertEquals("Author Unknown", entry.author);
    }
    {
      final ReferencesIterator.Entry entry = list.get(2);
      assertEquals("hvsc:/GAMES/file.sid#%233", entry.location);
      assertEquals(TimeStamp.createFrom(6789, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Escaped%21", entry.title);
      assertEquals("Me%)", entry.author);
    }
  }

  @Test
  public void parseMobileV2() throws IOException {
    final ArrayList<ReferencesIterator.Entry> list = XspfIterator.parse(getPlaylistResource(
        "mobile_v2"));
    assertEquals(3, list.size());
    {
      final ReferencesIterator.Entry entry = list.get(0);
      assertEquals("file:/folder/only%20duration", entry.location);
      assertEquals(TimeStamp.createFrom(123456, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("", entry.title);
      assertEquals("", entry.author);
    }
    {
      final ReferencesIterator.Entry entry = list.get(1);
      assertEquals("joshw:/some/%D1%84%D0%B0%D0%B9%D0%BB.7z#%D0%BF%D0%BE%D0%B4%D0%BF%D0%B0%D0%BF%D0%BA%D0%B0%2Fsubfile.mp3", entry.location);
      assertEquals(TimeStamp.createFrom(2345, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Название", entry.title);
      assertEquals("Author Unknown", entry.author);
    }
    {
      final ReferencesIterator.Entry entry = list.get(2);
      assertEquals("hvsc:/GAMES/file.sid#%233", entry.location);
      assertEquals(TimeStamp.createFrom(6789, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Escaped%21", entry.title);
      assertEquals("Me%)", entry.author);
    }
  }

  @Test
  public void parseDesktop() throws IOException {
    final ArrayList<ReferencesIterator.Entry> list = XspfIterator.parse(getPlaylistResource(
        "desktop"));
    assertEquals(2, list.size());
    {
      final ReferencesIterator.Entry entry = list.get(0);
      assertEquals("chiptunes/RP2A0X/nsfe/SuperContra.nsfe#%232", entry.location);
      assertEquals(TimeStamp.createFrom(111300, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Stage 1 - Lightning and Grenades", entry.title);
      assertEquals("H.Maezawa", entry.author);
    }
    {
      final ReferencesIterator.Entry entry = list.get(1);
      assertEquals("../chiptunes/DAC/ZX/dst/Untitled.dst", entry.location);
      assertEquals(TimeStamp.createFrom(186560, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Untitled2(c)&(p)Cj.Le0'99aug", entry.title);
      assertEquals("", entry.author);
    }
  }

  @Test
  public void createEmpty() throws IOException {
    final ByteArrayOutputStream out = new ByteArrayOutputStream(1024);
    {
      final Builder builder = new Builder(out);
      builder.writePlaylistProperties("Пустой", null);
      builder.finish();
    }
    assertEquals(getPlaylistReference("empty"), out.toString());
  }

  @Test
  public void createFull() throws IOException {
    final ByteArrayOutputStream out = new ByteArrayOutputStream(1024);
    {
      final Builder builder = new Builder(out);
      builder.writePlaylistProperties("Полный", 3);
      builder.writeTrack(new Item(createIdentifier("file:/folder/only duration"), "", "",
          TimeStamp.createFrom(123456, TimeUnit.MILLISECONDS)));
      builder.writeTrack(new Item(createIdentifier("joshw:/some/файл.7z#подпапка/subfile.mp3"),
          "Название", "Author Unknown", TimeStamp.createFrom(2345, TimeUnit.MILLISECONDS)));
      builder.writeTrack(new Item(createIdentifier("hvsc:/GAMES/file.sid##3"),
          "Escaped%21", "Me%)", TimeStamp.createFrom(6789, TimeUnit.MILLISECONDS)));
      builder.finish();
    }
    assertEquals(getPlaylistReference("mobile_v2"), out.toString());
  }

  private static Identifier createIdentifier(String decoded) {
    // To force encoding
    final Uri uri = Uri.parse(decoded);
    return new Identifier(new Uri.Builder()
        .scheme(uri.getScheme()).path(uri.getPath()).fragment(uri.getFragment()).build());
  }
}
