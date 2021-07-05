package app.zxtune.playback.service

import app.zxtune.TimeStamp
import app.zxtune.core.Player
import app.zxtune.sound.SamplesSource

internal class SeekableSamplesSource(private val player: Player) : SamplesSource {

    init {
        player.position = TimeStamp.EMPTY
    }

    override fun getSamples(buf: ShortArray) = if (player.render(buf)) {
        true
    } else {
        player.position = TimeStamp.EMPTY
        false
    }

    override var position: TimeStamp by player::position
}
