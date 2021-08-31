package app.zxtune.preferences;

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Bundle;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class DataStoreTest {

  @Test
  public void testDataStore() {
    ProviderClient client = mock(ProviderClient.class);

    DataStore store = new DataStore(client);

    Bundle cache = new Bundle();
    cache.putBoolean("none.boolean", true);
    cache.putInt("none.int", 234);
    cache.putLong("none.long", 567);
    cache.putString("none.string", "none");

    when(client.getAll()).thenReturn(cache);

    Assert.assertFalse(store.getBoolean("zxtune.boolean", false));
    Assert.assertTrue(store.getBoolean("none.boolean", false));
    Assert.assertEquals(123, store.getInt("zxtune.int", 123));
    Assert.assertEquals(234, store.getInt("none.int", 23));
    Assert.assertEquals(456, store.getLong("zxtune.long", 456));
    Assert.assertEquals(567, store.getLong("none.long", 56));
    Assert.assertEquals("test", store.getString("zxtune.string", "test"));
    Assert.assertEquals("none", store.getString("none.string", "default"));

    store.putBoolean("none.boolean", false);
    store.putInt("none.int", 34);
    store.putLong("zxtune.long", 56);
    store.putString("zxtune.string", "stored");

    Bundle bundle = new Bundle();
    bundle.putBoolean("zxtune.boolean", true);
    bundle.putInt("zxtune.int", 12);
    bundle.putLong("none.long", 67);
    bundle.putString("none.string", "empty");
    store.putBatch(bundle);

    Assert.assertEquals(12, store.getInt("zxtune.int", 0));
    Assert.assertEquals(34, store.getInt("none.int", 0));
    Assert.assertEquals(56, store.getLong("zxtune.long", 0));
    Assert.assertEquals(67, store.getLong("none.long", 0));
    Assert.assertEquals("stored", store.getString("zxtune.string", ""));
    Assert.assertEquals("empty", store.getString("none.string", "default"));

    {
      Assert.assertEquals(8, cache.size());
      Assert.assertEquals(12, cache.getInt("zxtune.int", 0));
      Assert.assertEquals(34, cache.getInt("none.int", 0));
      Assert.assertEquals(56, cache.getLong("zxtune.long", 0));
      Assert.assertEquals(67, cache.getLong("none.long", 0));
      Assert.assertEquals("stored", cache.getString("zxtune.string", ""));
      Assert.assertEquals("empty", cache.getString("none.string", "default"));
    }

    verify(client).getAll();
    verify(client).set(eq("none.boolean"), eq(false));
    verify(client).set(eq("none.int"), eq(34));
    verify(client).set(eq("zxtune.long"), eq(56L));
    verify(client).set(eq("zxtune.string"), eq("stored"));
    verify(client).set(eq(bundle));
  }
}
