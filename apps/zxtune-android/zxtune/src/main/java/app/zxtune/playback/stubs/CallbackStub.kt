/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback.stubs

import app.zxtune.TimeStamp
import app.zxtune.playback.Callback
import app.zxtune.playback.Item
import app.zxtune.playback.PlaybackControl

abstract class CallbackStub : Callback {
    override fun onInitialState(state: PlaybackControl.State) {}
    override fun onStateChanged(state: PlaybackControl.State, pos: TimeStamp) {}
    override fun onItemChanged(item: Item) {}
    override fun onError(e: String) {}
}
