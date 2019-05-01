package app.zxtune.core.jni;

import static org.junit.Assert.*;

import android.support.annotation.NonNull;
import org.junit.*;

import java.util.HashSet;

// TODO: Not working due to lack of JNI support in instrumentation tests
@Ignore
public class PluginsTest {

  @Test
  public void testPlugins() {
    final int[] flags = {0, 0};
    final int[] counts = {0, 0};
    final HashSet<String> players = new HashSet<>();
    final HashSet<String> containers = new HashSet<>();
    Plugins.enumerate(new Plugins.Visitor() {
      @Override
      public void onPlayerPlugin(int devices, @NonNull String id, @NonNull String description) {
        flags[0] |= devices;
        ++counts[0];
        players.add(id);
      }

      @Override
      public void onContainerPlugin(int type, @NonNull String id, @NonNull String description) {
        flags[1] |= 1 << type;
        ++counts[1];
        containers.add(id);
      }
    });

    assertTrue(flags[0] == 16383);
    assertTrue(flags[1] == 0b11011);
  }
}