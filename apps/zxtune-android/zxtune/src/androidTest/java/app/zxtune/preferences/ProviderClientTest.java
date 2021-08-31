package app.zxtune.preferences;

import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import app.zxtune.Preferences;

@RunWith(AndroidJUnit4.class)
public class ProviderClientTest {

  @Before
  public void setup() {
    SharedPreferences prefs = Preferences.getDefaultSharedPreferences(ApplicationProvider.getApplicationContext());

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
  }

  @Test
  public void testProviderClient() {
    ProviderClient client = new ProviderClient(ApplicationProvider.getApplicationContext());

    {
      Bundle all = client.getAll();
      Assert.assertEquals(9, all.size());
      Assert.assertTrue(all.getBoolean("zxtune.boolean", false));
      Assert.assertFalse(all.getBoolean("none.boolean", false));
      Assert.assertEquals(123, all.getInt("zxtune.int", 0));
      Assert.assertEquals(23, all.getInt("none.int", 23));
      Assert.assertEquals(456, all.getLong("zxtune.long", 0));
      Assert.assertEquals(56, all.getLong("none.long", 56));
      Assert.assertEquals("test", all.getString("zxtune.string", ""));
      Assert.assertEquals("7890", all.getString("zxtune.long_in_string", ""));
      Assert.assertEquals("ignored1", all.getString("ignored.string", ""));
      Assert.assertEquals("default", all.getString("none.string", "default"));
    }

    {
      client.set("none.boolean", true);
      client.set("none.int", 234);
      client.set("zxtune.long", 45L);
      client.set("zxtune.string", "str");
      Bundle bundle = new Bundle();
      bundle.putLong("none.long", 567L);
      bundle.putString("none.string", "none");
      bundle.putBoolean("zxtune.boolean", false);
      bundle.putInt("zxtune.int", 12);
      client.set(bundle);
    }

    Assert.assertEquals(13, client.getAll().size());
    {
      Bundle none = client.get("none.");
      Assert.assertEquals(4, none.size());

      Assert.assertTrue(none.getBoolean("none.boolean", false));
      Assert.assertEquals(234, none.getInt("none.int", 23));
      Assert.assertEquals(567, none.getLong("none.long", 56));
      Assert.assertEquals("none", none.getString("none.string", "default"));
    }
    {
      Bundle zxtune = client.get("zxtune.");
      Assert.assertEquals(5, zxtune.size());

      Assert.assertFalse(zxtune.getBoolean("zxtune.boolean", true));
      Assert.assertEquals(12, zxtune.getInt("zxtune.int", 23));
      Assert.assertEquals(45, zxtune.getLong("zxtune.long", 56));
      Assert.assertEquals("str", zxtune.getString("zxtune.string", "default"));
    }

    //TODO: test removal
  }
}
