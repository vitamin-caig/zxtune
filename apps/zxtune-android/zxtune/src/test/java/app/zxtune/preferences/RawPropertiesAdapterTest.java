package app.zxtune.preferences;

import app.zxtune.core.PropertiesModifier;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mockito;
import org.robolectric.RobolectricTestRunner;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

@RunWith(RobolectricTestRunner.class)
public class RawPropertiesAdapterTest {
  @Test
  public void testRawProperties() {
    PropertiesModifier target = Mockito.mock(PropertiesModifier.class);

    RawPropertiesAdapter adapter = new RawPropertiesAdapter(target);

    adapter.setProperty("ignored", 1);
    adapter.setProperty("zxtune.int", 123);
    adapter.setProperty("zxtune.long", 456L);
    adapter.setProperty("zxtune.bool", true);
    adapter.setProperty("zxtune.string", "test");
    adapter.setProperty("zxtune.long_in_string", "-45678");
    adapter.setProperty("zxtune.long_not_parsed", "90z");

    verify(target).setProperty(eq("zxtune.int"), eq(123L));
    verify(target).setProperty(eq("zxtune.long"), eq(456L));
    verify(target).setProperty(eq("zxtune.bool"), eq(1L));
    verify(target).setProperty(eq("zxtune.string"), eq("test"));
    verify(target).setProperty(eq("zxtune.long_in_string"), eq(-45678L));
    verify(target).setProperty(eq("zxtune.long_not_parsed"), eq("90z"));
    verifyNoMoreInteractions(target);
  }
}
