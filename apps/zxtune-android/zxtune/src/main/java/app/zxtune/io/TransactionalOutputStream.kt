package app.zxtune.io

import app.zxtune.Logger
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.io.OutputStream

private val LOG = Logger(TransactionalOutputStream::class.java.name)

class TransactionalOutputStream(private val target: File) : OutputStream() {
    private val temporary: File
    private var delegate: OutputStream?
    private var confirmed = false

    init {
        LOG.d { "Write cached file ${target.absolutePath}" }
        temporary = File("${target.path}~${hashCode()}")
        if (target.parentFile!!.mkdirs()) {
            LOG.d { "Created cache dir" }
        }
        delegate = FileOutputStream(temporary)
    }

    override fun close() {
        delegate!!.close()
        delegate = null
        if (confirmed) {
            commit()
        } else if (!temporary.delete()) {
            throw IOException("Failed to cleanup unconfirmed $temporary")
        }
    }

    private fun commit() {
        if (!Io.rename(temporary, target)) {
            val msg = "Failed to rename $temporary to $target"
            throw if (temporary.delete()) {
                IOException(msg)
            } else {
                IOException(msg, IOException("Failed to delete temporary $temporary"))
            }
        }
    }

    override fun flush() {
        onDelegate { flush() }
        confirmed = true
    }

    override fun write(b: ByteArray) = onDelegate {
        write(b)
    }

    override fun write(b: ByteArray, off: Int, len: Int) = onDelegate {
        write(b, off, len)
    }

    override fun write(b: Int) = onDelegate {
        write(b)
    }

    private fun onDelegate(op: OutputStream.() -> Unit) =
        try {
            op(delegate!!)
        } catch (e: IOException) {
            confirmed = false
            throw e
        }
}