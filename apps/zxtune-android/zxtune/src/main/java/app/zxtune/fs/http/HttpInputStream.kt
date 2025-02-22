package app.zxtune.fs.http

import java.io.InputStream
import java.net.HttpURLConnection
import java.util.zip.GZIPInputStream

internal class HttpInputStream(connection: HttpURLConnection) : InputStream() {

    private var connection: HttpURLConnection? = connection

    // Just for tests - no transparent compression...
    // https://developer.android.com/reference/java/net/HttpURLConnection.html#performance :
    // By default, this implementation of HttpURLConnection requests that servers use gzip
    // compression and it automatically decompresses the data for callers of
    // URLConnection.getInputStream(). The Content-Encoding and Content-Length response headers
    // are cleared in this case.
    private var delegate: InputStream? = if ("gzip" == connection.contentEncoding) {
        GZIPInputStream(connection.inputStream)
    } else {
        connection.inputStream
    }

    override fun close() {
        delegate = null
        connection?.disconnect()
        connection = null
    }

    override fun available() = delegate!!.available()

    override fun read() = delegate!!.read()

    override fun read(b: ByteArray, off: Int, len: Int) = delegate!!.read(b, off, len)

    override fun read(b: ByteArray) = delegate!!.read(b)

    override fun skip(n: Long) = delegate!!.skip(n)
}
