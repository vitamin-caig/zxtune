package app.zxtune.ui.utils

import android.os.Bundle
import android.os.Parcelable
import androidx.fragment.app.Fragment
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import app.zxtune.utils.getParcelableCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import kotlin.reflect.KProperty

object FragmentIntProperty {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        owner.requireArguments().getInt(property.name)

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: Int) {
        val args = owner.arguments ?: Bundle()
        owner.arguments = args.apply {
            putInt(property.name, value)
        }
    }
}

object FragmentStringProperty {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        owner.requireArguments().getString(property.name)

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: String) {
        val args = owner.arguments ?: Bundle()
        owner.arguments = args.apply {
            putString(property.name, value)
        }
    }
}

object FragmentLongArrayProperty {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        owner.requireArguments().getLongArray(property.name)

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: LongArray?) {
        val args = owner.arguments ?: Bundle()
        owner.arguments = args.apply {
            putLongArray(property.name, value)
        }
    }
}

internal class FragmentParcelableProperty<T : Parcelable> internal constructor(
    private val type: Class<T>,
) {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        requireNotNull(owner.requireArguments().getParcelableCompat(property.name, type))

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: T) {
        val args = owner.arguments ?: Bundle()
        owner.arguments = args.apply {
            putParcelable(property.name, value)
        }
    }

    internal companion object {
        internal inline fun <reified T : Parcelable> of() = FragmentParcelableProperty(T::class.java)
    }
}

fun LifecycleOwner.whenLifecycleStarted(block: suspend CoroutineScope.() -> Unit) {
    lifecycleScope.launch {
        repeatOnLifecycle(Lifecycle.State.STARTED, block)
    }
}
