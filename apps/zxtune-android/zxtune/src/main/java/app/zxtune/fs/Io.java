package app.zxtune.fs;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

import app.zxtune.Log;

public class Io {
    private static final String TAG = Io.class.getName();

    public static ByteBuffer readFrom(File file) throws IOException {
      final FileInputStream stream = new FileInputStream(file);
      try {
        final FileChannel channel = stream.getChannel();
        try {
          return readFrom(channel);
        } finally {
          channel.close();
        }
      } finally {
        stream.close();
      }
    }

    private static ByteBuffer readFrom(FileChannel channel) throws IOException {
      final int MIN_MMAPED_FILE_SIZE = 131072;
      if (channel.size() >= MIN_MMAPED_FILE_SIZE) {
        try {
          return readMemoryMapped(channel);
        } catch (IOException e) {
          Log.w(TAG, e, "Failed to read using MMAP. Use fallback");
          //http://stackoverflow.com/questions/8553158/prevent-outofmemory-when-using-java-nio-mappedbytebuffer
          System.gc();
          System.runFinalization();
        }
      }
      return readDirectArray(channel);
    }

    private static ByteBuffer readMemoryMapped(FileChannel channel) throws IOException {
      return channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
    }

    private static ByteBuffer readDirectArray(FileChannel channel) throws IOException {
      final ByteBuffer direct = ByteBuffer.allocateDirect((int) channel.size());
      channel.read(direct);
      direct.position(0);
      return direct;
    }

    public static void writeTo(File file, ByteBuffer data) throws IOException {
      final FileOutputStream stream = new FileOutputStream(file);
      try {
        final FileChannel chan = stream.getChannel();
        try {
          chan.write(data);
        } finally {
          data.position(0);
          chan.close();
        }
      } finally {
        stream.close();
      }
    }
}
