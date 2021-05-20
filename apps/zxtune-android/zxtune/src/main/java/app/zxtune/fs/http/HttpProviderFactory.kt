package app.zxtune.fs.http

import android.content.Context
import android.net.ConnectivityManager
import app.zxtune.R
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

        private val manager = context.getSystemService(Context.CONNECTIVITY_SERVICE) as?
                ConnectivityManager

        override fun hasConnection() = manager?.activeNetworkInfo?.isConnected ?: false

        override fun checkConnectionError() {
            if (!hasConnection()) {
                throw IOException(context.getString(R.string.network_inaccessible))
            }
        }
    }
}