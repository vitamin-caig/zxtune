/**
 *
 * @file
 *
 * @brief Ayl file parser
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.net.Uri;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.LinkedList;

import app.zxtune.io.Io;

public final class AylIterator {

  private static final String SIGNATURE = "ZX Spectrum Sound Chip Emulator Play List File v1.";
  private static final String PARAMETERS_BEGIN = "<";
  private static final String PARAMETERS_END = ">";
  
  public static ReferencesIterator create(InputStream stream) throws IOException {
    return new ReferencesArrayIterator(parse(stream));
  }

  private static class Properties {
    
    //private final int version;
    
    Properties(String id) throws IOException {
      if (!id.startsWith(SIGNATURE)) {
        throw new IOException("Invalid ayl signature");
      }
      final char ver = id.charAt(SIGNATURE.length());
      if (!Character.isDigit(ver)) {
        throw new IOException("Invalid ayl verison");
      }
      //this.version = ver - '0';  
    }
    
    /*
    final String getEncoding() {
      return version >= 6 ? "UTF-8" : "windows-1251";
    }
    */
  }
  
  public static ArrayList<ReferencesIterator.Entry> parse(InputStream stream) throws IOException {
    final BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
    /*final Properties props = */new Properties(reader.readLine());
    //TODO: apply encoding
    return parse(reader);
  }

  private static ArrayList<ReferencesIterator.Entry> parse(BufferedReader reader) throws IOException {
    final LinkedList<String> strings = readStrings(reader);
    final ArrayList<ReferencesIterator.Entry> result = new ArrayList<>(strings.size());
    parseAllParameters(strings);
    while (!strings.isEmpty()) {
      final ReferencesIterator.Entry entry = new ReferencesIterator.Entry();
      entry.location = Uri.encode(strings.removeFirst().replace('\\', '/'), "/");
      parseAllParameters(strings);
      result.add(entry);
    }
    return result;
  }
  
  private static LinkedList<String> readStrings(BufferedReader reader) throws IOException {
    final LinkedList<String> res = new LinkedList<>();
    String line;
    while ((line = reader.readLine()) != null) {
      res.add(line.trim());
    }
    reader.close();
    return res;
  }

  private static void parseAllParameters(LinkedList<String> strings) {
    for (;;) {
      if (!parseParameters(strings)) {
        break;
      }
    }
  }
  
  private static boolean parseParameters(LinkedList<String> strings) {
    if (strings.isEmpty() || !PARAMETERS_BEGIN.equals(strings.getFirst())) {
      return false;
    }
    strings.removeFirst();
    while (!strings.isEmpty()) {
      final String str = strings.removeFirst();
      if (PARAMETERS_END.equals(str)) {
        break;
      }
      //TODO: parse parameters string
    }
    return true;
  }
}
