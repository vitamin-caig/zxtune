/**
 * @file
 * @brief Http access helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.http

import android.net.Uri
import android.os.Build
import app.zxtune.TimeStamp
import app.zxtune.net.Http
import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection

internal class HttpUrlConnectionProvider(
    private val policy: Policy
) : HttpProvider {

    internal interface Policy {
        fun hasConnection(): Boolean

        @Throws(IOException::class)
        fun checkConnectionError()
    }

    override fun hasConnection() = policy.hasConnection()

    override fun getObject(uri: Uri): HttpObject =
        try {
            SimpleHttpObject(uri)
        } catch (e: IOException) {
            policy.checkConnectionError()
            throw e
        }

    private inner class SimpleHttpObject(uri: Uri) : HttpObject {

        private val _contentLength: Long?
        private val _lastModified: TimeStamp?
        private val _uri: Uri
        private val acceptRanges: Boolean

        init {
            val connection = openHeadConnection(uri)
            val code = connection.responseCode
            if (code != HttpURLConnection.HTTP_OK) {
                throw IOException("Unexpected code $code (${connection.responseMessage}) for HEAD request to $uri")
            }
            _contentLength = getContentLength(connection).takeIf { it >= 0 }
            _lastModified = getLastModified(connection)
            //may be different from original uri
            _uri = Uri.parse(connection.url.toString())
            acceptRanges = hasAcceptRanges(connection.headerFields)
            connection.disconnect()
        }

        override val uri = _uri

        override val contentLength = _contentLength

        override val lastModified = _lastModified

        override val input = contentLength?.let { limit ->
            FixedSizeInputStream(_uri, limit, acceptRanges)
        } ?: createStream(_uri, 0)
    }

    override fun getInputStream(uri: Uri) = createStream(uri, 0)

    private fun createStream(uri: Uri, offset: Long) =
        try {
            val connection = Http.createConnection(uri.toString())
            if (offset != 0L) {
                connection.setRequestProperty("Range", "bytes=${offset}-")
            }
            HttpInputStream(connection)
        } catch (e: IOException) {
            policy.checkConnectionError()
            throw e
        }

    private inner class FixedSizeInputStream(
        private val uri: Uri,
        private val total: Long,
        private val acceptRanges: Boolean
    ) : InputStream() {

        private var done: Long = 0
        private var delegate: HttpInputStream = createStream()

        override fun close() = delegate.close()

        override fun available() = delegate.available()

        override fun read(): Int {
            while (done < total) {
                val res = delegate.read()
                if (res == -1) {
                    reopenDelegate()
                } else {
                    ++done
                    return res
                }
            }
            return -1
        }

        override fun read(b: ByteArray, offset: Int, length: Int): Int {
            var off = offset
            var len = length
            if (done == total) {
                return -1
            }
            val doneBefore = done
            while (len > 0 && done < total) {
                val res = delegate.read(b, off, len)
                if (res < 0) {
                    reopenDelegate()
                } else {
                    done += res.toLong()
                    off += res
                    len -= res
                }
            }
            return (done - doneBefore).toInt()
        }

        override fun skip(amount: Long): Long {
            var n = amount
            val doneBefore = done
            while (n > 0 && done < total) {
                val res = delegate.skip(n)
                if (res == 0L) {
                    reopenDelegate()
                } else {
                    done += res
                    n -= res
                }
            }
            return done - doneBefore
        }

        private fun reopenDelegate() = with(createStream()) {
            close()
            delegate = this
        }

        private fun createStream(): HttpInputStream =
            if (done != 0L && !acceptRanges) {
                throw IOException("Got $done out of $total bytes while reading from $uri")
            } else {
                createStream(uri, done)
            }
    }
}

private fun openHeadConnection(uri: Uri): HttpURLConnection {
    var currentUri = uri.toString()
    var doneRedirects = 0
    while (true) {
        val connection = Http.createConnection(currentUri)
        connection.setRequestProperty("Accept-Encoding", "identity")
        connection.requestMethod = "HEAD"
        // Required to perform input in order to read response code and other
        //connection.doInput = false
        connection.instanceFollowRedirects = true
        connection.connect()
        if (isRedirect(connection.responseCode) && doneRedirects++ < 5) {
            currentUri = connection.getHeaderField("Location")
            connection.disconnect()
            continue
        }
        return connection
    }
}

private fun isRedirect(code: Int) =
    code == HttpURLConnection.HTTP_MOVED_PERM || code == HttpURLConnection.HTTP_MOVED_TEMP || code == HttpURLConnection.HTTP_SEE_OTHER

private fun getContentLength(connection: HttpURLConnection) =
    if (Build.VERSION.SDK_INT >= 24) {
        connection.contentLengthLong
    } else {
        connection.contentLength.toLong()
    }

private fun getLastModified(connection: HttpURLConnection): TimeStamp? =
    connection.lastModified.takeIf { it > 0 }?.let {
        TimeStamp.fromMilliseconds(it)
    }

private fun hasAcceptRanges(fields: Map<String?, List<String?>>): Boolean {
    for ((name, values) in fields) {
        if (0 == name?.compareTo("accept-ranges", ignoreCase = true)) {
            return "bytes" in values
        }
    }
    return false
}
