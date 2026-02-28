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
import app.zxtune.TimeStamp
import app.zxtune.core.PropertiesContainer
import app.zxtune.playback.service.PlaybackServiceImpl
import app.zxtune.preferences.Preferences
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.StateFlow

interface PlaybackService : Releaseable {
    val playbackControl: PlaybackControl
    val seekControl: SeekControl
    val visualizer: Visualizer

    val playbackProperties: PropertiesContainer?

    fun setNowPlaying(uri: Uri)

    val state : Flow<Pair<PlaybackControl.State, TimeStamp>>
    val nowPlaying : StateFlow<Item?>

    companion object {
        fun create(context: Context) =
            PlaybackServiceImpl(context, Preferences.getDataStore(context))
    }
}
