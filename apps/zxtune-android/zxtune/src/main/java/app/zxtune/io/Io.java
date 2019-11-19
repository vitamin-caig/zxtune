package app.zxtune.io;

import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.io.*;
import java.nio.BufferOverflowException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

import app.zxtune.Log;

public class Io {
  private static final String TAG = Io.class.getName();

  public static final int MIN_MMAPED_FILE_SIZE = 131072;
  public static final int INITIAL_BUFFER_SIZE = 262144;

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
    if (channel.size() >= MIN_MMAPED_FILE_SIZE) {
      for (int retry = 1; ; ++retry) {
        try {
          return readMemoryMapped(channel);
        } catch (IOException e) {
          if (retry == 1) {
            Log.w(TAG, e, "Failed to read using MMAP. Cleanup memory");
            //http://stackoverflow.com/questions/8553158/prevent-outofmemory-when-using-java-nio-mappedbytebuffer
            System.gc();
            System.runFinalization();
          } else {
            Log.w(TAG, e, "Failed to read using MMAP. Fallback");
            break;
          }
        }
      }
    }
    return readDirectArray(channel);
  }

  private static ByteBuffer readMemoryMapped(FileChannel channel) throws IOException {
    return channel.map(FileChannel.MapMode.READ_ONLY, 0, validateSize(channel.size()));
  }

  private static ByteBuffer readDirectArray(FileChannel channel) throws IOException {
    final ByteBuffer direct = allocateDirectBuffer(channel.size());
    channel.read(direct);
    direct.position(0);
    return direct;
  }

  private static long validateSize(long size) throws IOException {
    if (size == 0) {
      throw new IOException("Empty file");
    } else {
      return size;
    }
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

  @NonNull
  public static ByteBuffer readFrom(@NonNull InputStream stream) throws IOException {
    try {
      byte[] buffer = reallocate(null);
      int size = 0;
      for (; ; ) {
        size = readPartialContent(stream, buffer, size);
        if (size == buffer.length) {
          buffer = reallocate(buffer);
        } else {
          break;
        }
      }
      validateSize(size);
      return ByteBuffer.wrap(buffer, 0, size);
    } finally {
      stream.close();
    }
  }

  @NonNull
  public static ByteBuffer readFrom(@NonNull InputStream stream, long size) throws IOException {
    try {
      final ByteBuffer result = allocateDirectBuffer(size);
      final byte[] buffer = reallocate(null);
      int totalSize = 0;
      for (; ; ) {
        final int partSize = readPartialContent(stream, buffer, 0);
        if (partSize != 0) {
          totalSize += partSize;
          result.put(buffer, 0, partSize);
          if (partSize == buffer.length) {
            continue;
          }
        }
        break;
      }
      if (totalSize != size) {
        throw new IOException("File size mismatch");
      }
      result.position(0);
      return result;
    } catch (BufferOverflowException err) {
      throw new IOException("File size mismatch", err);
    } finally {
      stream.close();
    }
  }

  private static int readPartialContent(@NonNull InputStream stream, @NonNull byte[] buffer, int offset) throws IOException {
    final int len = buffer.length;
    while (offset < len) {
      final int chunk = stream.read(buffer, offset, len - offset);
      if (chunk < 0) {
        break;
      }
      offset += chunk;
    }
    return offset;
  }

  @NonNull
  private static byte[] reallocate(@Nullable byte[] buf) throws IOException {
    for (int retry = 1; ; ++retry) {
      try {
        if (buf != null) {
          final byte[] result = new byte[buf.length * 3 / 2];
          System.arraycopy(buf, 0, result, 0, buf.length);
          return result;
        } else {
          return new byte[INITIAL_BUFFER_SIZE];
        }
      } catch (OutOfMemoryError err) {
        if (retry == 1) {
          Log.d(TAG, "Retry reallocate call for OOM");
          System.gc();
        } else {
          throw new IOException(err);
        }
      }
    }
  }

  @NonNull
  private static ByteBuffer allocateDirectBuffer(long size) throws IOException {
    validateSize(size);
    for (int retry = 1; ; ++retry) {
      try {
        return ByteBuffer.allocateDirect((int) size);
      } catch (OutOfMemoryError err) {
        if (retry == 1) {
          Log.d(TAG, "Retry reallocate call for OOM");
          System.gc();
        } else {
          throw new IOException(err);
        }
      }
    }
  }

  @NonNull
  public static String readHtml(@NonNull InputStream in) throws IOException {
    final ByteBuffer buf = readFrom(in);
    return new String(buf.array(), 0, buf.limit(), "UTF-8");
  }

  public static long copy(@NonNull InputStream in, @NonNull OutputStream out) throws IOException {
    try {
      final byte[] buffer = reallocate(null);
      for (long total = 0; ; ) {
        final int size = readPartialContent(in, buffer, 0);
        out.write(buffer, 0, size);
        total += size;
        if (size != buffer.length) {
          return total;
        }
      }
    } finally {
      in.close();
    }
  }

  public static void touch(File file) {
    if (!file.setLastModified(System.currentTimeMillis())) {
      try {
        final RandomAccessFile raf = new RandomAccessFile(file, "rw");
        final long size = raf.length();
        raf.setLength(size + 1);
        raf.setLength(size);
        raf.close();
      } catch (IOException e) {
        Log.w(TAG, e, "Failed to update file timestamp");
      }
    }
  }

  public static boolean rename(File oldName, File newName) {
    if (Build.VERSION.SDK_INT >= 26) {
      return renameViaFiles(oldName, newName);
    } else {
      return oldName.renameTo(newName)
          || (newName.exists() && newName.delete() && oldName.renameTo(newName));
    }
  }

  @RequiresApi(26)
  private static boolean renameViaFiles(File oldName, File newName) {
    try {
      Files.move(oldName.toPath(), newName.toPath(), StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
      return true;
    } catch (IOException e) {
      Log.w(TAG, e, "Failed to rename '%s' -> '%s'", oldName.getAbsolutePath(), newName.getAbsolutePath());
    }
    return false;
  }
}
