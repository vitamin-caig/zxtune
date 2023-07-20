package app.zxtune.device

import android.content.Context
import android.content.Intent
import android.os.PowerManager
import android.provider.Settings
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.core.content.getSystemService
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import app.zxtune.Features

interface PowerManagement {
    val hasProblem: LiveData<Boolean>
    fun updateState()
    fun createFixitIntent(): Intent

    companion object {
        private lateinit var instance: PowerManagement

        @JvmStatic
        fun dozeEnabled(ctx: Context) = if (Features.DozeMode.isAvailable()) {
            createDozeManagement(ctx).hasProblem.value
        } else {
            null
        }

        fun create(ctx: Context): PowerManagement {
            if (!this::instance.isInitialized) {
                instance = createImpl(ctx.applicationContext)
            }
            return instance
        }

        private fun createImpl(ctx: Context) = when {
            Features.DozeMode.isAvailable() -> createDozeManagement(ctx)
            else -> StubManagement
        }

        @RequiresApi(Features.DozeMode.REQUIRED_SDK)
        private fun createDozeManagement(ctx: Context) = DozeModeManagement(
            requireNotNull(ctx.getSystemService()),
            ctx.packageName
        )
    }
}

@RequiresApi(Features.DozeMode.REQUIRED_SDK)
@VisibleForTesting
class DozeModeManagement(private val pm: PowerManager, private val pkg: String) : PowerManagement {

    private val state by lazy {
        MutableLiveData(currentState)
    }

    private val currentState
        get() = !pm.isIgnoringBatteryOptimizations(pkg)

    override val hasProblem
        get() = state

    override fun updateState() {
        state.value = currentState
    }

    override fun createFixitIntent() = Intent().apply {
        addCategory(Intent.CATEGORY_DEFAULT)
        action = Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS
    }
}

@VisibleForTesting
object StubManagement : PowerManagement {

    private val state = object : LiveData<Boolean>(false) {}

    override val hasProblem
        get() = state

    override fun updateState() = Unit
    override fun createFixitIntent() = throw IllegalStateException("Should not be called")
}
