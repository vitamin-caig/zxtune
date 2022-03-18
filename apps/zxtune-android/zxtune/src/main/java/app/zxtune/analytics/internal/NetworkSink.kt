package app.zxtune.analytics.internal

import app.zxtune.fs.api.Api

internal class NetworkSink : UrlsSink {
    override fun push(url: String) = Api.postEvent(url)
}
