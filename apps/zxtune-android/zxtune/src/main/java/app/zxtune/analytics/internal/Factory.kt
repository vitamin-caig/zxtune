package app.zxtune.analytics.internal

import android.content.Context

object Factory {
    @JvmStatic
    fun createClientEndpoint(ctx: Context): UrlsSink {
        return ProviderClient(ctx)
    }
}
