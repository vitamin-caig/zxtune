package app.zxtune.fs

import android.net.Uri

abstract class TestObject(val idx: Int) : VfsObject {
    // Value should be persistent across calls
    private val _parent by lazy {
        if (idx > 0) TestDir(idx - 1) else null
    }
    override val uri: Uri = Uri.parse("schema:/authority/path?${idx}")
    override val name = "object${idx}"
    override val description = "Object $idx"
    override val parent : VfsObject?
        get() = _parent

    override fun getExtension(id: String): Any? = null
}

internal open class TestDir(idx: Int) : VfsDir, TestObject(idx) {
    override fun enumerate(visitor: VfsDir.Visitor): Unit = TODO("Should not be called")
}

internal open class TestFile(idx: Int, override val size: String) : VfsFile, TestObject(idx)
