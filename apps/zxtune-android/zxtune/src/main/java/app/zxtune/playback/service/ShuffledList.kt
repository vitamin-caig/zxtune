package app.zxtune.playback.service

internal class ShuffledList<T>(val compare: (T, T) -> Boolean = { l, h -> l == h }) {
    private var storage: ArrayList<T> = ArrayList() // index -> data
    private var _remapping: IntArray? = null // sequence -> index
    private val remapping
        get() = if (shuffled) {
            _remapping ?: IntArray(storage.size) { it }.apply { shuffle() }.also { _remapping = it }
        } else null
    var shuffled = false

    class Cursor<T>(val sequence: Int, val index: Int, val data: T) {
        init {
            require(sequence >= 0)
            require(index >= 0)
        }

        override fun toString() = "Cursor(s/i=$sequence/$index, $data)"
    }

    val size
        get() = storage.size

    fun update(data: ArrayList<T>) {
        storage = data
        _remapping = null
    }

    fun replace(cur: Cursor<T>, replace: (T) -> T?) = findIndex(cur)?.let {
        replace(storage[it])?.let { newOne ->
            storage[it] = newOne
            true
        }
    } ?: false

    fun getOrNull(cur: Cursor<T>) = findIndex(cur)?.let { storage[it] }

    fun find(predicate: (T) -> Boolean) =
        storage.indexOfFirst(predicate).takeIf { it != -1 }?.let { index ->
            val sequence = findSequence(index)
            Cursor(sequence, index, storage[index])
        }

    fun advanceCursor(cur: Cursor<T>, delta: Int) = findIndex(cur)?.let { idx ->
        val seq = if (idx == cur.index) cur.sequence else findSequence(idx)
        val nextSeq = seq + delta
        if (nextSeq in storage.indices) {
            val nextIdx = remapping?.get(nextSeq) ?: nextSeq
            Cursor(nextSeq, nextIdx, storage[nextIdx])
        } else {
            null
        }
    }

    private fun findIndex(cur: Cursor<T>) =
        cur.index.takeIf { it in storage.indices && compare(storage[it], cur.data) }
            ?: storage.indexOfFirst { compare(it, cur.data) }.takeIf { it != -1 }

    private fun findSequence(index: Int) = remapping?.indexOf(index) ?: index
}
