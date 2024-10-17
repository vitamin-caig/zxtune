package app.zxtune.device

import android.content.Context
import android.os.PowerManager
import android.provider.Settings
import app.zxtune.Features
import app.zxtune.assertThrows
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [Features.DozeMode.REQUIRED_SDK])
class PowerManagementTestDoze {

    private val powerManager = mock<PowerManager>()
    private val packageName = "package.name"
    private val context = mock<Context> {
        on { applicationContext } doReturn mock
        on { getSystemService(PowerManager::class.java) } doReturn powerManager
        on { packageName } doReturn packageName
    }

    @Test
    fun `regular workflow`() {
        var dozeEnabled = true
        powerManager.stub {
            on { isIgnoringBatteryOptimizations(packageName) } doAnswer { !dozeEnabled }
        }
        with(PowerManagement.create(context)) {
            assert(this is DozeModeManagement)

            assertEquals(true, PowerManagement.dozeEnabled(context))
            assertEquals(true, hasProblem.value)
            createFixitIntent().run {
                assertEquals(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS, action)
            }

            dozeEnabled = false
            assertEquals(false, PowerManagement.dozeEnabled(context))
            assertEquals(true, hasProblem.value)
            updateState()
            assertEquals(false, hasProblem.value)
        }
    }
}

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [Features.DozeMode.REQUIRED_SDK - 1])
class PowerManagementTestStub {

    private val context = mock<Context> {
        on { applicationContext } doReturn mock
    }

    @Test
    fun `regular workflow`() {
        assertEquals(null, PowerManagement.dozeEnabled(context))
        with(PowerManagement.create(context)) {
            assert(this is StubManagement)
            assertEquals(false, hasProblem.value)
            updateState()
            assertEquals(false, hasProblem.value)
            assertThrows<IllegalStateException> { createFixitIntent() }
        }
    }
}
