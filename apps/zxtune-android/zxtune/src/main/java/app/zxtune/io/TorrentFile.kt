package app.zxtune.io

class TorrentFile(private val node: Node) {
    init {
        requireThat(node !is End)
    }

    sealed interface Node {
        infix operator fun get(key: String) = find(key) ?: throw INVALID_FORMAT
        fun find(key: String) = dict[key]
        fun forEach(block: (Node) -> Unit) = list.forEach(block)

        val str
            get() = (this as? StringNode)?.value ?: throw INVALID_FORMAT
        val int
            get() = (this as? IntegerNode)?.value ?: throw INVALID_FORMAT
        private val dict
            get() = (this as? DictNode)?.value ?: throw INVALID_FORMAT
        private val list
            get() = (this as? ListNode)?.value ?: throw INVALID_FORMAT
    }

    @JvmInline
    private value class StringNode(val value: String) : Node

    @JvmInline
    private value class IntegerNode(val value: Long) : Node

    @JvmInline
    private value class ListNode(val value: ArrayList<Node>) : Node

    @JvmInline
    private value class DictNode(val value: HashMap<String, Node>) : Node

    private data object End : Node

    fun forEachFile(cb: (String, Long) -> Unit) = node[KEY_INFO].let { info ->
        val name = info[KEY_NAME].str
        info.find(KEY_FILES)?.forEach { file ->
            val filename = StringBuilder().apply {
                append(name)
                file[KEY_PATH].forEach {
                    append('/')
                    append(it.str)
                }
            }.toString()
            cb(filename, file[KEY_LENGTH].int)
        } ?: cb(name, info[KEY_LENGTH].int)
    }

    companion object {
        private val INVALID_FORMAT = IllegalArgumentException("Invalid torrent file format")
        private fun requireThat(condition: Boolean) = if (!condition) {
            throw INVALID_FORMAT
        } else Unit

        private const val KEY_INFO = "info"
        private const val KEY_NAME = "name"
        private const val KEY_FILES = "files"
        private const val KEY_PATH = "path"
        private const val KEY_LENGTH = "length"

        fun parse(data: ByteArray) = TorrentFile(Input(data).readNode())
    }

    private class Input(private val data: ByteArray) {
        private var offset = 0

        fun readNode(): Node = when (readNext()) {
            END -> End
            INT -> IntegerNode(readInt()).also {
                requireThat(readNext() == END)
            }

            DICT -> HashMap<String, Node>().apply {
                while (true) {
                    val key = readNode()
                    if (key is End) break
                    put((key as StringNode).value, readNode())
                }
            }.run {
                DictNode(this)
            }

            LIST -> ArrayList<Node>().apply {
                while (true) {
                    val elem = readNode()
                    if (elem is End) break
                    add(elem)
                }
            }.run {
                ListNode(this)
            }

            in DIGITS -> {
                --offset
                val len = readInt()
                require(':'.code == readNext())
                StringNode(readString(len.toInt()))
            }

            else -> throw INVALID_FORMAT
        }

        private fun peekNext() =
            if (offset < data.size) data[offset].toInt() else throw INVALID_FORMAT

        private fun readNext() = peekNext().also { ++offset }

        fun readInt(): Long {
            var out = 0L
            requireThat(peekNext() in DIGITS)
            while (true) {
                val sym = peekNext()
                if (sym in DIGITS) {
                    out = out * 10 + (sym - '0'.code)
                    ++offset
                } else {
                    return out
                }
            }
        }

        fun readString(len: Int) = requireThat(offset + len <= data.size).run {
            String(data, offset, len)
        }.also {
            offset += len
        }

        companion object {
            const val END = 'e'.code
            const val INT = 'i'.code
            const val DICT = 'd'.code
            const val LIST = 'l'.code
            private val DIGITS = '0'.code..'9'.code
        }
    }
}
