package app.zxtune.fs.http

import java.io.InputStream
import java.net.HttpURLConnection

internal class HttpInputStream(connection: HttpURLConnection) : InputStream() {

    private var connection: HttpURLConnection? = connection
    private var delegate: InputStream? = connection.inputStream

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
