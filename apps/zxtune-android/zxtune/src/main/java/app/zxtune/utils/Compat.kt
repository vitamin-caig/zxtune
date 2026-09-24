package app.zxtune.utils

import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.os.Parcelable
import java.io.Serializable

/*
 * Bundle/Intent accessors got typed overloads in Android 13 (TIRAMISU). The untyped
 * ones are deprecated since then but are still the only option on older devices,
 * so route through the typed overloads when available and use suppressed legacy
 * accessors otherwise.
 */

@Suppress("DEPRECATION")
inline fun <reified T : Parcelable> Bundle.getParcelableCompat(key: String): T? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    getParcelable(key, T::class.java)
} else {
    getParcelable(key)
}

@Suppress("DEPRECATION")
fun <T : Parcelable> Bundle.getParcelableCompat(key: String, clazz: Class<T>): T? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    getParcelable(key, clazz)
} else {
    getParcelable(key)
}

@Suppress("DEPRECATION")
inline fun <reified T : Parcelable> Intent.getParcelableExtraCompat(key: String): T? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    getParcelableExtra(key, T::class.java)
} else {
    getParcelableExtra(key)
}

@Suppress("DEPRECATION", "UNCHECKED_CAST")
inline fun <reified T : Parcelable> Bundle.getParcelableArrayCompat(key: String): Array<T?>? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    getParcelableArray(key, T::class.java)
} else {
    getParcelableArray(key) as Array<T?>?
}

@Suppress("DEPRECATION", "UNCHECKED_CAST")
inline fun <reified T : Serializable> Bundle.getSerializableCompat(key: String): T? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    getSerializable(key, T::class.java)
} else {
    getSerializable(key) as T?
}

// Bundle elements are accessable by generic key-value contract only
@Suppress("DEPRECATION")
fun Bundle.getUntyped(key: String): Any? = get(key)
