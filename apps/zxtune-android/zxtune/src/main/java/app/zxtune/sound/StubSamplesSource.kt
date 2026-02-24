package app.zxtune.sound

import app.zxtune.TimeStamp

object StubSamplesSource : SamplesSource {
    override suspend fun getSamples(buf: ShortArray) = false

    override var position: TimeStamp
        get() = TimeStamp.EMPTY
        set(_) {}
}
