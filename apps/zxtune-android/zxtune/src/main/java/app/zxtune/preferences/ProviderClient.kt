package app.zxtune.preferences

import android.content.ContentResolver
import android.content.Context
import android.database.ContentObserver
import android.net.Uri
import android.os.Bundle
import androidx.lifecycle.LiveData
import app.zxtune.Releaseable
import app.zxtune.ReleaseableStub

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

    fun getLive(key: String, defaultValue: String = ""): LiveData<String> =
        PreferenceLiveData(resolver, key, defaultValue) { k, def ->
            getString(k, def)
        }

    fun getLive(key : String, defaultValue: Int = 0) : LiveData<Int> =
        PreferenceLiveData(resolver, key, defaultValue) { k, def ->
            getInt(k, def)
        }
}

private fun ContentResolver.list(prefix: String? = null) =
    call(Provider.URI, Provider.METHOD_LIST, prefix, null)

private fun ContentResolver.get(key: String) =
    call(Provider.URI, Provider.METHOD_GET, key, null)

private fun ContentResolver.put(data: Bundle) =
    call(Provider.URI, Provider.METHOD_PUT, null, data)

// Do not use generic getters to avoid type check on each call.
class PreferenceLiveData<T>(
    private val resolver: ContentResolver,
    private val key: String,
    private val defValue: T,
    private val getter: Bundle.(String, T) -> T,
) : LiveData<T>() {

    private var subscription: Releaseable = ReleaseableStub

    private val observer = object : ContentObserver(null) {
        override fun onChange(selfChange: Boolean) {
            postValue(getActualValue())
        }
    }

    override fun getValue() = if (isUpdating()) {
        super.getValue()
    } else {
        getActualValue()
    }

    private fun isUpdating() = hasActiveObservers()

    override fun onActive() {
        subscription = resolver.subscribe(Provider.notificationUri(key), observer)
        super.setValue(getActualValue())
    }

    override fun onInactive() = subscription.release().also { subscription = ReleaseableStub }

    private fun getActualValue() = resolver.get(key)?.getter(key, defValue) ?: defValue
}

// TODO: extract
private fun ContentResolver.subscribe(
    uri: Uri,
    observer: ContentObserver,
    notifyForDescendants: Boolean = false,
) = Releaseable { unregisterContentObserver(observer) }.also {
    registerContentObserver(uri, notifyForDescendants, observer)
}
