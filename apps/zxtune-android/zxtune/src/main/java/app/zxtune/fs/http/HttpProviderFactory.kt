package app.zxtune.fs.http

import android.content.Context
import app.zxtune.R
import app.zxtune.net.NetworkManager
import java.io.IOException

object HttpProviderFactory {
    @JvmStatic
    fun createProvider(ctx: Context): HttpProvider =
        HttpUrlConnectionProvider(policy = PolicyImpl(ctx))

    fun createTestProvider(): HttpProvider =
        HttpUrlConnectionProvider(object : HttpUrlConnectionProvider.Policy {
            override fun hasConnection() = true

            override fun checkConnectionError() = Unit
        })

    private class PolicyImpl(private val context: Context) : HttpUrlConnectionProvider.Policy {

        private val mgr = NetworkManager.from(context)

        override fun hasConnection() = mgr.networkAvailable.value!!

        override fun checkConnectionError() {
            if (!hasConnection()) {
                throw IOException(context.getString(R.string.network_inaccessible))
            }
        }
    }
}