package app.zxtune.analytics.internal

import android.content.ContentProvider
import android.content.ContentResolver
import android.content.ContentValues
import android.content.Context
import android.media.AudioManager
import android.media.AudioTrack
import android.net.Uri
import android.os.Build
import android.os.Bundle
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.auth.Auth
import app.zxtune.device.PowerManagement.Companion.dozeEnabled
import app.zxtune.fs.api.Api
import app.zxtune.net.NetworkManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

class Provider : ContentProvider() {
    @OptIn(ExperimentalCoroutinesApi::class)
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO.limitedParallelism(1))
    private lateinit var sink: UrlsSink

    override fun onCreate(): Boolean {
        val ctx = context ?: return false
        val appCtx = ctx.applicationContext
        MainApplication.initialize(appCtx)
        val auth = Auth.getUserInfo(ctx)
        Api.initialize(auth)
        sink = Dispatcher().apply {
            scope.launch {
                NetworkManager.from(appCtx).networkAvailable.collect(this@apply::onNetworkChange)
            }
        }

        sendSystemInfoEvent()
        sendSystemConfigurationEvent(ctx)
        if (auth.isInitial) {
            sendInitialInstallationEvent()
        }
        return true
    }

    override fun shutdown() {
        scope.cancel()
    }

    private fun sendSystemInfoEvent() = UrlsBuilder("system/info").apply {
        addParam("product", Build.PRODUCT)
        addParam("device", Build.DEVICE)
        supportedAbis().forEachIndexed { idx, abi ->
            addIndexedParam("cpu_abi", idx, abi)
        }
        addParam("manufacturer", Build.MANUFACTURER)
        addParam("brand", Build.BRAND)
        addParam("model", Build.MODEL)

        addParam("android", Build.VERSION.RELEASE)
        addParam("sdk", Build.VERSION.SDK_INT.toLong())

        addParam(
            "samplerate", AudioTrack.getNativeOutputSampleRate(AudioManager.STREAM_MUSIC).toLong()
        )
    }.run {
        doPush(result)
    }

    private fun sendSystemConfigurationEvent(ctx: Context) = UrlsBuilder("system/config").apply {
        val res = ctx.resources
        val cfg = res.configuration
        val metrics = res.displayMetrics
        addParam("layout", cfg.screenLayout.toLong())
        if (Build.VERSION.SDK_INT >= 24) {
            cfg.locales.run {
                for (idx in 0..<size()) {
                    addIndexedParam("locale", idx, get(idx).toString())
                }
            }
        } else {
            @Suppress("DEPRECATION") addParam("locale", cfg.locale.toString())
        }
        addParam("density", metrics.densityDpi.toLong())
        addParam("width", cfg.screenWidthDp.toLong())
        addParam("height", cfg.screenHeightDp.toLong())
        addParam("sw", cfg.smallestScreenWidthDp.toLong())
        dozeEnabled(ctx)?.let { doze ->
            addParam("doze", (if (doze) 1 else 0).toLong())
        }
    }.run {
        doPush(result)
    }

    private fun sendInitialInstallationEvent() = UrlsBuilder("app/firstrun").run {
        // TODO: add installation source
        doPush(result)
    }

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ) = null

    override fun getType(uri: Uri) = null

    override fun insert(uri: Uri, values: ContentValues?) = null

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) = 0

    override fun update(
        uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?
    ) = 0

    override fun call(method: String, arg: String?, extras: Bundle?): Bundle? {
        if (METHOD_PUSH == method && arg != null) {
            doPush(arg)
        }
        return null
    }

    private fun doPush(url: String) = scope.launch {
        runCatching {
            sink.push(url)
        }.onFailure {
            LOG.w(it) { "Failed to push $url" }
        }
    }

    companion object {
        private val LOG = Logger(Provider::class.java.name)

        const val METHOD_PUSH = "push"

        val URI: Uri = Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT)
            .authority("app.zxtune.analytics.internal").build()
    }
}

private fun UrlsBuilder.addIndexedParam(name: String, idx: Int, value: String) =
    addParam(if (idx == 0) name else name + (idx + 1), value)

@Suppress("DEPRECATION")
private fun supportedAbis() = if (Build.VERSION.SDK_INT >= 21) Build.SUPPORTED_ABIS else arrayOf(
    Build.CPU_ABI, Build.CPU_ABI2
)
