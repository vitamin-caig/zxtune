package app.zxtune.fs.http

import android.net.Uri
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import java.io.IOException
import java.util.*
import kotlin.math.min

private val LOG = Logger(MultisourceHttpProvider::class.java.name)
private const val MIN_QUARANTINE_PERIOD = 1000L
private const val MAX_QUARANTINE_PERIOD = 60 * 60 * 1000L

// TODO: operate with time value types
class MultisourceHttpProvider @VisibleForTesting internal constructor(
    private val delegate: HttpProvider,
    private val time: TimeSource
) {
    internal fun interface TimeSource {
        fun nowMillis(): Long
    }

    private val hostStat = HashMap<String, HostStatistics>()

    constructor(delegate: HttpProvider) : this(delegate, System::currentTimeMillis)

    fun hasConnection() = delegate.hasConnection()

    @Throws(IOException::class)
    fun getObject(uris: Array<Uri>) = getSingleObject(uris, delegate::getObject)

    @Throws(IOException::class)
    fun getInputStream(uri: Uri) = delegate.getInputStream(uri)

    @Throws(IOException::class)
    fun getInputStream(uris: Array<Uri>) = getSingleObject(uris, delegate::getInputStream)

    private fun <T> getSingleObject(uris: Array<Uri>, getter: (Uri) -> T): T {
        val now = time.nowMillis()
        for ((idx, uri) in uris.withIndex()) {
            val host: String = uri.host ?: continue
            val stat = getStat(host)
            val isLast = idx == uris.size - 1
            if (!isLast && stat.isDisabledFor(now)) {
                continue
            }
            try {
                val result = getter(uri)
                stat.onSuccess(now)
                return result
            } catch (ex: IOException) {
                if (isLast || !delegate.hasConnection()) {
                    throw ex
                } else {
                    stat.onError(now, IOException(ex))
                }
            }
        }
        throw IOException("No suitable urls found")
    }

    @VisibleForTesting
    fun isHostDisabledFor(host: String, time: Long) = hostStat[host]?.isDisabledFor(time) ?: false

    @Synchronized
    private fun getStat(host: String) = hostStat.getOrPut(host) { HostStatistics(host) }

    private class HostStatistics constructor(private val host: String) {
        private var successes = 0
        private var lastSuccess: Long = 0
        private var errors = 0
        private var lastError: Long = 0
        private var disabledTill: Long = 0

        @Synchronized
        fun isDisabledFor(now: Long) = disabledTill > now

        @Synchronized
        fun onSuccess(now: Long) {
            ++successes
            lastSuccess = now
        }

        fun onError(now: Long, ex: IOException) {
            if (disableFor(now)) {
                LOG.w(ex) { "Temporarily disable requests to '$host' (${successes}/${errors})" }
            } else {
                LOG.w(ex) { "Give a change after error for host '$host' (${successes}/${errors})" }
            }
        }

        @Synchronized
        private fun disableFor(now: Long): Boolean {
            val quarantine = calculateQuarantine()
            ++errors
            lastError = now
            disabledTill = now + quarantine
            return quarantine != 0L
        }

        // forgive first error in chain
        private fun calculateQuarantine() = when {
            // forgive first error in chain
            lastSuccess >= lastError -> 0
            successes >= errors -> MIN_QUARANTINE_PERIOD
            else -> min(MAX_QUARANTINE_PERIOD, MIN_QUARANTINE_PERIOD * (errors - successes))
        }
    }
}