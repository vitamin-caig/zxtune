package app.zxtune.playlist;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.LinkedList;

public final class AylIterator {

  private final static String SIGNATURE = "ZX Spectrum Sound Chip Emulator Play List File v1.";
  private final static String PARAMETERS_BEGIN = "<";
  private final static String PARAMETERS_END = ">";
  
  public static ReferencesIterator create(ByteBuffer buf) throws IOException {
    return new ReferencesArrayIterator(parse(buf));
  }

  private static class Properties {
    
    private final int version;
    
    Properties(String id) throws IOException {
      if (!id.startsWith(SIGNATURE)) {
        throw new IOException("Invalid ayl signature");
      }
      final char ver = id.charAt(SIGNATURE.length());
      if (!Character.isDigit(ver)) {
        throw new IOException("Invalid ayl verison");
      }
      this.version = ver - '0';  
    }
    
    final String getEncoding() {
      return version >= 6 ? "UTF-8" : "windows-1251";
    }
  }
  
  private static ArrayList<ReferencesIterator.Entry> parse(ByteBuffer buf) throws IOException {
    final InputStream stream = XspfIterator.newInputStream(buf);
    final BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
    final Properties props = new Properties(reader.readLine());
    //TODO: apply encoding
    return parse(reader);
  }

  private static ArrayList<ReferencesIterator.Entry> parse(BufferedReader reader) throws IOException {
    final LinkedList<String> strings = readStrings(reader);
    final ArrayList<ReferencesIterator.Entry> result = new ArrayList<ReferencesIterator.Entry>(strings.size());
    while (parseParameters(strings)) {};
    while (!strings.isEmpty()) {
      final ReferencesIterator.Entry entry = new ReferencesIterator.Entry();
      entry.location = strings.removeFirst().replace('\\', '/');
      while (parseParameters(strings)) {}
      result.add(entry);
    }
    return result;
  }
  
  private static LinkedList<String> readStrings(BufferedReader reader) throws IOException {
    final LinkedList<String> res = new LinkedList<String>();
    String line;
    while ((line = reader.readLine()) != null) {
      res.add(line.trim());
    }
    return res;
  }
  
  private static boolean parseParameters(LinkedList<String> strings) {
    if (strings.isEmpty() || !strings.getFirst().equals(PARAMETERS_BEGIN)) {
      return false;
    }
    strings.removeFirst();
    while (!strings.isEmpty()) {
      final String str = strings.removeFirst();
      if (str.equals(PARAMETERS_END)) {
        break;
      }
      //TODO: parse parameters string
    }
    return true;
  }
}
