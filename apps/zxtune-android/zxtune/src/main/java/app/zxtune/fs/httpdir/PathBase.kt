package app.zxtune.fs.httpdir

const val DELIMITER = '/'
const val DELIMITER_STR = DELIMITER.toString()

abstract class PathBase protected constructor(
    val elements: List<String>, val isDir: Boolean
) : Path {

    protected fun getRemoteId() = getLocalId().let { local ->
        if (isDir && local.isNotEmpty()) {
            local + DELIMITER
        } else {
            local
        }
    }

    protected abstract fun build(elements: List<String>, isDir: Boolean): Path

    override fun getLocalId() = elements.joinToString(DELIMITER_STR)

    override fun getName() = elements.lastOrNull().orEmpty()

    override fun getParent() = if (elements.isEmpty()) null else build(elements.dropLast(1), true)

    override fun getChild(name: String): Path {
        if (name.isEmpty() || name == DELIMITER_STR) {
            return this
        }
        val isAbsolute = name.startsWith(DELIMITER)
        val isDir = name.endsWith(DELIMITER)
        val from = if (isAbsolute) 1 else 0
        val to = name.length - (if (isDir) 1 else 0)
        val components = name.substring(from, to).split(DELIMITER)
        val allComponents = if (isAbsolute) {
            components
        } else {
            elements.toMutableList().apply {
                addAll(components)
            }
        }
        return build(allComponents, isDir)
    }

    override fun isEmpty() = elements.isEmpty()

    override fun isFile() = !isDir
}
