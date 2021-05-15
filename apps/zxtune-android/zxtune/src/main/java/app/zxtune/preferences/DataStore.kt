package app.zxtune.preferences

import android.content.Context
import android.os.Bundle
import androidx.annotation.VisibleForTesting
import androidx.preference.PreferenceDataStore

class DataStore @VisibleForTesting constructor(val client: ProviderClient) : PreferenceDataStore() {

    private val cache by lazy {
        client.all.also {
            cacheRef = it
        }
    }
    private var cacheRef: Bundle? = null

    constructor(ctx: Context) : this(ProviderClient(ctx))

    override fun putBoolean(key: String, value: Boolean) {
        client.set(key, value)
        cacheRef?.putBoolean(key, value)
    }

    override fun putInt(key: String, value: Int) {
        client.set(key, value)
        cacheRef?.putInt(key, value)
    }

    override fun putString(key: String, value: String?) = value?.let {
        client.set(key, it)
        cacheRef?.putString(key, it)
    } ?: Unit

    override fun putLong(key: String, value: Long) {
        client.set(key, value)
        cacheRef?.putLong(key, value)
    }

    fun putBatch(batch: Bundle) {
        client.set(batch)
        cacheRef?.putAll(batch)
    }

    override fun getString(key: String, defValue: String?): String? = cache.getString(key, defValue)

    override fun getInt(key: String, defValue: Int) = cache.getInt(key, defValue)

    override fun getLong(key: String, defValue: Long): Long = cache.getLong(key, defValue)

    override fun getBoolean(key: String, defValue: Boolean) = cache.getBoolean(key, defValue)

    override fun getStringSet(key: String, defValues: Set<String>?) =
        throw UnsupportedOperationException("Not implemented on this data store")

    override fun getFloat(key: String, defValue: Float) =
        throw UnsupportedOperationException("Not implemented on this data store")
}
