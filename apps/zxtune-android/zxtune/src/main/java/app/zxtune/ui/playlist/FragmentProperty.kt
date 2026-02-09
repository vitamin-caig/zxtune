package app.zxtune.ui.playlist

import android.os.Bundle
import androidx.fragment.app.Fragment
import app.zxtune.playlist.IO.getPlaylistOperationScope
import app.zxtune.playlist.IO.toBundle
import app.zxtune.playlist.Playlist
import app.zxtune.playlist.Track
import kotlin.reflect.KProperty

object FragmentPlaylistOperationScopeProperty {
    operator fun getValue(owner: Fragment, property: KProperty<*>) =
        owner.requireArguments().getPlaylistOperationScope()

    operator fun setValue(owner: Fragment, property: KProperty<*>, value: Playlist.OperationScope) {
        owner.arguments = value.toBundle()
    }
}

object FragmentTrackIdSetProperty {
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
