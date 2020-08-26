package app.zxtune.playlist.xspf;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.playlist.ReferencesIterator;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

@RunWith(AndroidJUnit4.class)
public class XspfPlaylistTest {

  private static InputStream getPlaylistResource(String name) {
    return XspfPlaylistTest.class.getClassLoader().getResourceAsStream("playlists/" + name +
        ".xspf");
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
  public void parseMobile() throws IOException {
    final ArrayList<ReferencesIterator.Entry> list = XspfIterator.parse(getPlaylistResource("mobile"));
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
      assertEquals("joshw:/some/with%20title", entry.location);
      assertEquals(TimeStamp.createFrom(234567, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Title 1", entry.title);
      assertEquals("", entry.author);
    }
    {
      final ReferencesIterator.Entry entry = list.get(2);
      assertEquals("hvsc:/GAMES/full", entry.location);
      assertEquals(TimeStamp.createFrom(345678, TimeUnit.MILLISECONDS), entry.duration);
      assertEquals("Title#2", entry.title);
      assertEquals("Creator", entry.author);
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
}
