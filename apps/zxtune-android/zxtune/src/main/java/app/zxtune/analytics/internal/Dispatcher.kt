package app.zxtune.analytics.internal

import app.zxtune.Logger
import java.io.IOException
import java.util.ArrayDeque

internal class Dispatcher : UrlsSink {
    private val online = NetworkSink()
    private val offline = BufferSink()
    private var current: UrlsSink = online
    private var networkRetryCountdown = 0

    override fun push(url: String) = try {
        current.push(url)
        trySwitchToNetwork()
    } catch (e: IOException) {
        LOG.w(e) { "Network error" }
        current = offline
        networkRetryCountdown = NETWORK_RETRIES_PERIOD
    }

    @Throws(IOException::class)
    private fun trySwitchToNetwork() {
        if (networkRetryCountdown > 0) {
            --networkRetryCountdown
            if (0 == networkRetryCountdown) {
                offline.flushTo(online)
                current = online
            }
        }
    }

    fun onNetworkChange(isAvailable: Boolean) {
        LOG.d { "onNetworkChange: $isAvailable" }
        if (isAvailable) {
            if (current === offline) {
                networkRetryCountdown = 1
            } else {
                current = online
            }
        } else {
            current = offline
            networkRetryCountdown = 0
        }
    }

    private class BufferSink : UrlsSink {
        private val buffers = ArrayDeque<ArrayDeque<String>>(MAX_BLOCKS_COUNT)
        private var size = 0
        private var lost = 0
        private var trimSize = 1

        override fun push(url: String) {
            val buf = buffers.peekLast()?.takeIf { it.size < MAX_BLOCK_SIZE } ?: allocateBuffer()
            buf.addLast(url)
            ++size
        }

        fun allocateBuffer() = ArrayDeque<String>().also {
            buffers.addLast(it)
            if (buffers.size > MAX_BLOCKS_COUNT) {
                trimBuffers()
            }
        }

        private fun trimBuffers() {
            for (i in trimSize downTo 1) {
                lost += buffers.removeFirst().size
            }
            size -= lost
            if (trimSize * 2 < MAX_BLOCKS_COUNT) {
                trimSize *= 2
            }
        }

        @Throws(IOException::class)
        fun flushTo(sink: UrlsSink) {
            if (size == 0) {
                return
            }
            sendDiagnostic(sink)
            while (!buffers.isEmpty()) {
                flush(buffers.first(), sink)
                buffers.removeFirst()
            }
        }

        @Throws(IOException::class)
        private fun sendDiagnostic(sink: UrlsSink) {
            sink.push(diagnosticUrl)
            lost = 0
        }

        private val diagnosticUrl
            get() = UrlsBuilder("stat/offline").apply {
                addParam("count", size.toLong())
                addParam("lost", lost.toLong())
            }.result

        @Throws(IOException::class)
        private fun flush(buf: ArrayDeque<String>, sink: UrlsSink) {
            val suffix = resendSuffix
            while (!buf.isEmpty()) {
                sink.push(buf.first() + suffix)
                buf.removeFirst()
                --size
            }
        }

        private val resendSuffix
            get() = "&rts=" + System.currentTimeMillis() / 1000

        companion object {
            private const val MAX_BLOCK_SIZE = 1000
            private const val MAX_BLOCKS_COUNT = 100
        }
    }

    companion object {
        private val LOG = Logger(Dispatcher::class.java.name)

        private const val NETWORK_RETRIES_PERIOD = 1000
    }
}
