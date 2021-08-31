package app.zxtune.preferences;

import static org.mockito.ArgumentMatchers.eq;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.os.Looper;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

import app.zxtune.Preferences;
import app.zxtune.Releaseable;
import app.zxtune.core.PropertiesModifier;

@RunWith(AndroidJUnit4.class)
public class SharedPreferencesBridgeTest {

  private SharedPreferences prefs;
  private PropertiesModifier target;

  @Before
  public void setup() {
    Context ctx = ApplicationProvider.getApplicationContext();
    prefs = Preferences.getDefaultSharedPreferences(ctx);

    SharedPreferences.Editor editor = prefs.edit();
    editor.clear();
    editor.putBoolean("zxtune.boolean", true);
    editor.putInt("zxtune.int", 123);
    editor.putLong("zxtune.long", 456);
    editor.putString("zxtune.string", "test");
    editor.putString("zxtune.long_in_string", "7890");
    editor.putString("ignored.string", "ignored1");
    editor.putBoolean("ignored.boolean", false);
    editor.putInt("ignored.int", 23);
    editor.putLong("ignored.long", 56);
    editor.commit();

    target = Mockito.mock(PropertiesModifier.class);
  }

  @After
  public void tearDown() {
    Mockito.verifyNoMoreInteractions(target);
  }

  @Test
  public void testInitialProperties() {
    Releaseable hook = SharedPreferencesBridge.subscribe(prefs, target);

    Mockito.verify(target).setProperty(eq("zxtune.boolean"), eq(1L));
    Mockito.verify(target).setProperty(eq("zxtune.int"), eq(123L));
    Mockito.verify(target).setProperty(eq("zxtune.long"), eq(456L));
    Mockito.verify(target).setProperty(eq("zxtune.string"), eq("test"));
    Mockito.verify(target).setProperty(eq("zxtune.long_in_string"), eq(7890L));
  }

  @Test
  public void testUpdates() {
    Releaseable hook = SharedPreferencesBridge.subscribe(prefs, target);

    Mockito.reset(target);

    {
      SharedPreferences.Editor editor = prefs.edit();
      editor.putFloat("ignored.float", 1.0f);
      editor.putBoolean("ignored.boolean", false);
      editor.putBoolean("zxtune.boolean", false);
      editor.putBoolean("zxtune.boolean2", true);
      editor.putString("ignored.string", "ignored");
      editor.putString("zxtune.string", "update");
      editor.putString("zxtune.string2", "new");
      editor.putInt("ignored.int", 111);
      editor.putInt("zxtune.int", 234);
      editor.putInt("zxtune.int2", 345);
      editor.putLong("ignored.log", 222);
      editor.putLong("zxtune.long", 567);
      editor.putLong("zxtune.long2", 678);
      editor.commit();
    }
    waitForIdle();

    hook.release();

    {
      SharedPreferences.Editor editor = prefs.edit();
      editor.putString("zxtune.string", "skipped");
      editor.commit();
    }
    waitForIdle();

    Mockito.verify(target).setProperty(eq("zxtune.boolean"), eq(0L));
    Mockito.verify(target).setProperty(eq("zxtune.boolean2"), eq(1L));
    Mockito.verify(target).setProperty(eq("zxtune.int"), eq(234L));
    Mockito.verify(target).setProperty(eq("zxtune.int2"), eq(345L));
    Mockito.verify(target).setProperty(eq("zxtune.long"), eq(567L));
    Mockito.verify(target).setProperty(eq("zxtune.long2"), eq(678L));
    Mockito.verify(target).setProperty(eq("zxtune.string"), eq("update"));
    Mockito.verify(target).setProperty(eq("zxtune.string2"), eq("new"));
  }

  private static void waitForIdle() {
    final Handler handler = new Handler(Looper.getMainLooper());
    try {
      synchronized(handler) {
        handler.post(() -> {synchronized(handler) {handler.notifyAll();}});
        handler.wait();
      }
    } catch (InterruptedException e) {
    }
  }
}
