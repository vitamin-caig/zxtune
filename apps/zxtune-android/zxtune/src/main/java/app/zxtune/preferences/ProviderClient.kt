package app.zxtune.preferences

import android.content.ContentResolver
import android.content.Context
import android.os.Bundle
import app.zxtune.ui.utils.observeChanges
import kotlinx.coroutines.flow.transform

class ProviderClient(ctx: Context) {

    private val resolver = ctx.contentResolver

    val all
        get() = requireNotNull(resolver.list())

    fun get(prefix: String) = resolver.list(prefix)

    fun set(key: String, value: String) = set(Bundle().apply { putString(key, value) })

    fun set(key: String, value: Int) = set(Bundle().apply { putInt(key, value) })

    fun set(key: String, value: Long) = set(Bundle().apply { putLong(key, value) })

    fun set(key: String, value: Boolean) = set(Bundle().apply { putBoolean(key, value) })

    fun set(batch: Bundle) = resolver.put(batch)

    fun watchString(key: String, defaultValue: String? = null) = watch(key, defaultValue) { k, _ ->
        getString(k)
    }

    fun watchInt(key: String, defaultValue: Int? = null) = watch(key, defaultValue) { k, def ->
        when {
            def != null -> getInt(k, def)
            containsKey(k) -> getInt(k)
            else -> defaultValue
        }
    }

    private fun <T> watch(key: String, defaultValue: T?, fetch: Bundle.(String, T?) -> T?) =
        resolver.observeChanges(Provider.notificationUri(key), false).transform {
            val value = resolver.get(key)?.fetch(key, defaultValue) ?: defaultValue
            value?.let {
                emit(it)
            }
        }
}

private fun ContentResolver.list(prefix: String? = null) =
    call(Provider.URI, Provider.METHOD_LIST, prefix, null)

private fun ContentResolver.get(key: String) = call(Provider.URI, Provider.METHOD_GET, key, null)

private fun ContentResolver.put(data: Bundle) = call(Provider.URI, Provider.METHOD_PUT, null, data)
