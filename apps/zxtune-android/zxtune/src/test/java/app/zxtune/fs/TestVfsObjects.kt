package app.zxtune.fs

import android.net.Uri

abstract class TestObject(val idx: Int) : VfsObject {
    override val uri: Uri = Uri.parse("schema:/authority/path?${idx}")
    override val name = "object${idx}"
    override val description = "Object $idx"
    override val parent : VfsObject?
        get() = TODO("Should not be called")

    override fun getExtension(id: String): Any? = null
}

internal open class TestDir(idx: Int) : VfsDir, TestObject(idx) {
    override fun enumerate(visitor: VfsDir.Visitor): Unit = TODO("Should not be called")
}

internal open class TestFile(idx: Int, override val size: String) : VfsFile, TestObject(idx)
