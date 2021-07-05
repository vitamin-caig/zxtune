package app.zxtune.sound

import app.zxtune.TimeStamp

object StubSamplesSource : SamplesSource {
    override fun getSamples(buf: ShortArray) = false

    override var position: TimeStamp
        get() = TimeStamp.EMPTY
        set(_) {}

    //TODO: remove
    @JvmStatic
    fun instance() = StubSamplesSource
}
