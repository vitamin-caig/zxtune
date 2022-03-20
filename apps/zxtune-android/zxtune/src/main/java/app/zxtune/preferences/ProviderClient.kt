package app.zxtune.preferences

import android.content.ContentResolver
import android.content.Context
import android.os.Bundle

// open for mocking
open class ProviderClient(ctx: Context) {

    private val resolver: ContentResolver = ctx.contentResolver

    open val all: Bundle
        get() = resolver.call(Provider.URI, Provider.METHOD_LIST, null, null)!!

    open fun get(prefix: String) = resolver.call(Provider.URI, Provider.METHOD_LIST, prefix, null)

    open fun set(key: String, value: String) = set(Bundle().apply { putString(key, value) })

    open fun set(key: String, value: Int) = set(Bundle().apply { putInt(key, value) })

    open fun set(key: String, value: Long) = set(Bundle().apply { putLong(key, value) })

    open fun set(key: String, value: Boolean) = set(Bundle().apply { putBoolean(key, value) })

    open fun set(batch: Bundle) = resolver.call(Provider.URI, Provider.METHOD_PUT, null, batch)
}
