package app.zxtune.ui.playlist

import android.os.Bundle
import androidx.fragment.app.Fragment
import app.zxtune.playlist.Track
import kotlin.reflect.KProperty

object FragmentIdSetProperty {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        owner.requireArguments().getLongArray(property.name)?.let {
            Track.IdSet(it)
        }

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: Track.IdSet?) {
        val args = owner.arguments ?: Bundle()
        owner.arguments = args.apply {
            putLongArray(property.name, value?.storage)
        }
    }
}
