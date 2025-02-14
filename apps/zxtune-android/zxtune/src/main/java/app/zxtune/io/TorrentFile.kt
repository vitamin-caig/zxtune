package app.zxtune.io

import androidx.annotation.VisibleForTesting

// May require fix int parsing for full specification support
class TorrentFile(@get:VisibleForTesting val node: Node) {
    init {
        require(node !is End)
    }

    sealed interface Node {
        infix operator fun get(key: String) = requireNotNull(find(key))
        fun find(key: String) = dict[key]
        fun forEach(block: (Node) -> Unit) = list.forEach(block)

        val str
            get() = requireNotNull((this as? StringNode)?.value)
        val int
            get() = requireNotNull((this as? IntegerNode)?.value)
        private val dict
            get() = requireNotNull((this as? DictNode)?.value)
        private val list
            get() = requireNotNull((this as? ListNode)?.value)
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
                require(readNext() == END)
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

            else -> throw IllegalArgumentException()
        }

        private fun peekNext() =
            if (offset < data.size) data[offset].toInt() else throw IllegalArgumentException()

        private fun readNext() = peekNext().also { ++offset }

        fun readInt(): Long {
            var out = 0L
            var sign = 1L
            when (val sym = readNext()) {
                '-'.code -> {
                    sign = -1L
                }

                '0'.code -> {
                    return 0
                }

                else -> {
                    require(sym in DIGITS)
                    out = (sym - '0'.code).toLong()
                }
            }
            while (true) {
                val sym = peekNext()
                if (sym in DIGITS) {
                    out = out * 10 + (sym - '0'.code)
                    ++offset
                } else {
                    require(out > 0)
                    return out * sign
                }
            }
        }

        fun readString(len: Int) = require(offset + len <= data.size).run {
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
