package app.zxtune.analytics.internal

import android.content.Context
import app.zxtune.fs.api.Api

internal class NetworkSink(ctx: Context) : UrlsSink {

    init {
        Api.initialize(ctx)
    }

    override fun push(url: String) = Api.postEvent(url)
}
