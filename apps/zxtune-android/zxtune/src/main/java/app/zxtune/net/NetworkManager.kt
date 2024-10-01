package app.zxtune.net

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.LiveData
import androidx.startup.AppInitializer
import androidx.startup.Initializer
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.ReleaseableStub

private val LOG = Logger(NetworkManager::class.java.name)

class NetworkManager @VisibleForTesting constructor(src: NetworkStateSource) {

    val networkAvailable: LiveData<Boolean> = ConnectionState(src)

    init {
        LOG.d { "Created" }
    }

    companion object {
        @JvmStatic
        fun from(ctx: Context) =
            AppInitializer.getInstance(ctx).initializeComponent(Factory::class.java)
    }

    class Factory : Initializer<NetworkManager> {
        override fun create(ctx: Context) = NetworkManager(NetworkStateSource.create(ctx))
        override fun dependencies(): List<Class<out Initializer<*>>> = emptyList()
    }
}

typealias NetworkStateCallback = (Boolean) -> Unit

@VisibleForTesting
interface NetworkStateSource {
    fun isNetworkAvailable(): Boolean
    fun monitorChanges(cb: NetworkStateCallback): Releaseable

    companion object {
        fun create(ctx: Context) = if (Build.VERSION.SDK_INT >= 24) {
            NetworkStateApi24(ctx)
        } else {
            NetworkStatePre24(ctx)
        }
    }
}

private class ConnectionState(private val source: NetworkStateSource) : LiveData<Boolean>() {

    private var subscription: Releaseable = ReleaseableStub

    override fun getValue() = if (isUpdating()) {
        super.getValue()
    } else {
        source.isNetworkAvailable()
    }

    private fun isUpdating() = hasActiveObservers()

    override fun onActive() {
        subscription = source.monitorChanges(this::update).also { subscription.release() }
        value = source.isNetworkAvailable()
    }

    override fun onInactive() = subscription.also { subscription = ReleaseableStub }.release()

    private fun update(newState: Boolean) = postValue(newState).also {
        LOG.d { "NetworkState=${newState}" }
    }
}

// Based on solutions from
// https://stackoverflow.com/questions/36421930/connectivitymanager-connectivity-action-deprecated
@RequiresApi(24)
private class NetworkStateApi24(ctx: Context) : NetworkStateSource {

    private val manager = ctx.getConnectivityManager()

    override fun isNetworkAvailable() = manager?.activeNetwork?.let { network ->
        manager.getNetworkCapabilities(network)?.run {
            hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) && hasCapability(
                NetworkCapabilities.NET_CAPABILITY_VALIDATED
            )
        }
    } ?: false

    override fun monitorChanges(cb: NetworkStateCallback): Releaseable {
        val networkCallback = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) = cb.invoke(true)
            override fun onLost(network: Network) = cb.invoke(false)
        }
        manager?.registerDefaultNetworkCallback(networkCallback)
        return Releaseable { manager?.unregisterNetworkCallback(networkCallback) }
    }
}

@Suppress("DEPRECATION")
private class NetworkStatePre24(private val ctx: Context) : NetworkStateSource {

    override fun isNetworkAvailable() =
        true == ctx.getConnectivityManager()?.activeNetworkInfo?.isConnected

    override fun monitorChanges(cb: NetworkStateCallback): Releaseable {
        val receiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                val isNoConnectivity =
                    intent?.extras?.getBoolean(ConnectivityManager.EXTRA_NO_CONNECTIVITY) ?: true
                cb.invoke(!isNoConnectivity)
            }
        }
        ctx.registerReceiver(receiver, IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION))
        return Releaseable { ctx.unregisterReceiver(receiver) }
    }
}

private fun Context.getConnectivityManager() =
    getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
