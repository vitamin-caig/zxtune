package app.zxtune.playback.service

import app.zxtune.core.Player
import app.zxtune.playback.Visualizer

internal class PlaybackVisualizer(private val player: Player) : Visualizer {
    override fun getSpectrum(levels: ByteArray) = player.analyze(levels)
}
