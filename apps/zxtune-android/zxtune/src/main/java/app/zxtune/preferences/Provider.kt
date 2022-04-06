package app.zxtune.preferences

import android.content.ContentProvider
import android.content.ContentResolver
import android.content.ContentValues
import android.content.SharedPreferences
import android.database.Cursor
import android.net.Uri
import android.os.Bundle
import androidx.core.content.edit
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.Preferences

private val LOG = Logger(Provider::class.java.name)

class Provider : ContentProvider() {

    private val prefs by lazy {
        Preferences.getDefaultSharedPreferences(requireNotNull(context)).apply {
            registerOnSharedPreferenceChangeListener(changeListener)
        }
    }

    // should be strong reference
    private val changeListener =
        SharedPreferences.OnSharedPreferenceChangeListener { _, key ->
            key?.let {
                resolver.notifyChange(notificationUri(it), null)
            }
        }
    private val resolver
        get() = requireNotNull(context).contentResolver

    override fun onCreate() = context?.let { ctx ->
        MainApplication.initialize(ctx.applicationContext)
        true
    } ?: false

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ): Cursor? = null

    override fun getType(uri: Uri): String? = null

    override fun insert(uri: Uri, values: ContentValues?): Uri? = null

    @Synchronized
    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?): Int {
        selectionArgs?.let { delete(it) }
        return 0
    }

    override fun update(
        uri: Uri,
        values: ContentValues?,
        selection: String?,
        selectionArgs: Array<String>?
    ) = 0

    @Synchronized
    override fun call(method: String, arg: String?, extras: Bundle?): Bundle? = when (method) {
        METHOD_GET -> arg?.let { get(it) }
        METHOD_LIST -> list(arg)
        METHOD_PUT -> extras?.let {
            put(it)
            null
        }
        else -> null
    }

    private fun get(key: String) = prefs.all[key]?.let { value ->
        Bundle().apply {
            copyPref(key, value, this)
        }
    }

    private fun list(prefix: String?) = Bundle().apply {
        for ((key, value) in prefs.all) {
            if (prefix == null || key.startsWith(prefix)) {
                copyPref(key, value!!, this)
            }
        }
    }

    private fun put(data: Bundle) = prefs.edit(commit = false) {
        for (key in data.keySet()) {
            data[key]?.let { value ->
                setPref(key, value, this)
            }
        }
    }

    private fun setPref(key: String, value: Any, editor: SharedPreferences.Editor) = with(editor) {
        LOG.d { "${key}=${value}" }
        when (value) {
            is Long -> putLong(key, value)
            is Int -> putInt(key, value)
            is String -> putString(key, value)
            is Boolean -> putBoolean(key, value)
            else -> throw IllegalArgumentException("Invalid type for $key: ${value.javaClass.name}")
        }
    }

    private fun delete(keys: Array<String>) = prefs.edit(commit = false) {
        for (k in keys) {
            remove(k)
        }
    }

    companion object {
        val URI: Uri = builder().build()

        fun notificationUri(key: String): Uri = builder().appendPath(key).build()

        private fun builder() = Uri.Builder()
            .scheme(ContentResolver.SCHEME_CONTENT)
            .authority("app.zxtune.preferences")

        const val METHOD_GET = "get"
        const val METHOD_LIST = "list"
        const val METHOD_PUT = "put"
    }
}

private fun copyPref(key: String, value: Any, result: Bundle) = with(result) {
    when (value) {
        is Long -> putLong(key, value)
        is Int -> putInt(key, value)
        is String -> putString(key, value)
        is Boolean -> putBoolean(key, value)
        else -> throw IllegalArgumentException("Invalid type for $key: ${value.javaClass.name}")
    }
}
