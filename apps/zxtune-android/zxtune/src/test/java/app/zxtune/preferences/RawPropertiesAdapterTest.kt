package app.zxtune.preferences

import app.zxtune.core.PropertiesModifier
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RawPropertiesAdapterTest {
    @Test
    fun `test RawPropertiesAdapter`() {
        val target = mock<PropertiesModifier>()
        with(RawPropertiesAdapter(target)) {
            setProperty("ignored", 1)
            setProperty("zxtune.int", 123)
            setProperty("zxtune.long", 456L)
            setProperty("zxtune.bool", true)
            setProperty("zxtune.string", "test")
            setProperty("zxtune.long_in_string", "-45678")
            setProperty("zxtune.long_not_parsed", "90z")
        }

        verify(target).setProperty(eq("zxtune.int"), eq(123L))
        verify(target).setProperty(eq("zxtune.long"), eq(456L))
        verify(target).setProperty(eq("zxtune.bool"), eq(1L))
        verify(target).setProperty(eq("zxtune.string"), eq("test"))
        verify(target).setProperty(eq("zxtune.long_in_string"), eq(-45678L))
        verify(target).setProperty(eq("zxtune.long_not_parsed"), eq("90z"))
        verifyNoMoreInteractions(target)
    }
}