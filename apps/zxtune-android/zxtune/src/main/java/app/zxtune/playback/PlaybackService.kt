/**
 *
 * @file
 *
 * @brief Playback service interface
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

import android.content.Context
import android.net.Uri
import app.zxtune.Releaseable
import app.zxtune.core.PropertiesContainer
import app.zxtune.playback.service.PlaybackServiceImpl
import app.zxtune.preferences.Preferences

interface PlaybackService : Releaseable {
    val playbackControl: PlaybackControl
    val seekControl: SeekControl
    val visualizer: Visualizer

    val playbackProperties: PropertiesContainer?

    val nowPlaying: Item

    fun restoreSession()
    fun setNowPlaying(uri: Uri)

    fun subscribe(cb: Callback): Releaseable

    companion object {
        fun create(context: Context) =
            PlaybackServiceImpl(context, Preferences.getDataStore(context))
    }
}
