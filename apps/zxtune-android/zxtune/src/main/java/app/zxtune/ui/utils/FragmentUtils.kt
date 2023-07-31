package app.zxtune.ui.utils

import android.os.Bundle
import android.os.Parcelable
import androidx.fragment.app.Fragment
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

class FragmentParcelableProperty<T : Parcelable> {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        requireNotNull(owner.requireArguments().getParcelable<T>(property.name))

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: T) {
        val args = owner.arguments ?: Bundle()
        owner.arguments = args.apply {
            putParcelable(property.name, value)
        }
    }
}
