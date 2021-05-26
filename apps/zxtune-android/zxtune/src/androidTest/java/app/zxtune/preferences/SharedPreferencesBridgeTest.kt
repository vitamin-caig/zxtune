package app.zxtune.preferences

import android.content.Context
import android.content.SharedPreferences
import android.os.Handler
import android.os.Looper
import androidx.core.content.edit
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import app.zxtune.Preferences
import app.zxtune.core.PropertiesModifier
import app.zxtune.preferences.SharedPreferencesBridge.subscribe
import app.zxtune.use
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

@RunWith(AndroidJUnit4::class)
class SharedPreferencesBridgeTest {

    private lateinit var prefs: SharedPreferences
    private lateinit var target: PropertiesModifier

    @Before
    fun setup() {
        val ctx = ApplicationProvider.getApplicationContext<Context>()
        prefs = Preferences.getDefaultSharedPreferences(ctx)
        prefs.edit(true) {
            clear()
            putBoolean("zxtune.boolean", true)
            putInt("zxtune.int", 123)
            putLong("zxtune.long", 456)
            putString("zxtune.string", "test")
            putString("zxtune.long_in_string", "7890")
            putString("ignored.string", "ignored1")
            putBoolean("ignored.boolean", false)
            putInt("ignored.int", 23)
            putLong("ignored.long", 56)
        }
        target = mock()
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(target)
    }

    @Test
    fun testInitialProperties() = subscribe(prefs, target).use {
        verify(target).setProperty(eq("zxtune.boolean"), eq(1L))
        verify(target).setProperty(eq("zxtune.int"), eq(123L))
        verify(target).setProperty(eq("zxtune.long"), eq(456L))
        verify(target).setProperty(eq("zxtune.string"), eq("test"))
        verify(target).setProperty(eq("zxtune.long_in_string"), eq(7890L))
    }

    @Test
    fun testUpdates() {
        subscribe(prefs, target).use {
            reset(target)
            prefs.edit(true) {
                putFloat("ignored.float", 1.0f)
                putBoolean("ignored.boolean", false)
                putBoolean("zxtune.boolean", false)
                putBoolean("zxtune.boolean2", true)
                putString("ignored.string", "ignored")
                putString("zxtune.string", "update")
                putString("zxtune.string2", "new")
                putInt("ignored.int", 111)
                putInt("zxtune.int", 234)
                putInt("zxtune.int2", 345)
                putLong("ignored.log", 222)
                putLong("zxtune.long", 567)
                putLong("zxtune.long2", 678)
                remove("zxtune.long")
            }
            waitForIdle()
        }
        prefs.edit(true) {
            putString("zxtune.string", "skipped")
        }
        waitForIdle()
        verify(target).setProperty(eq("zxtune.boolean"), eq(0L))
        verify(target).setProperty(eq("zxtune.boolean2"), eq(1L))
        verify(target).setProperty(eq("zxtune.int"), eq(234L))
        verify(target).setProperty(eq("zxtune.int2"), eq(345L))
        //verify(target).setProperty(eq("zxtune.long"), eq(567L));
        verify(target).setProperty(eq("zxtune.long2"), eq(678L))
        verify(target).setProperty(eq("zxtune.string"), eq("update"))
        verify(target).setProperty(eq("zxtune.string2"), eq("new"))
    }
}

private fun waitForIdle() {
    val handler = Handler(Looper.getMainLooper())
    ReentrantLock().run {
        val condition = newCondition()
        withLock {
            handler.post {
                withLock { condition.signalAll() }
            }
            condition.await()
        }
    }
}
