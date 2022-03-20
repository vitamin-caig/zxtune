package app.zxtune.fs.httpdir

import android.content.Context
import androidx.annotation.VisibleForTesting
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.CommandExecutor
import app.zxtune.fs.dbhelpers.FileTree
import app.zxtune.fs.dbhelpers.QueryCommand
import app.zxtune.fs.httpdir.Catalog.DirVisitor
import java.util.*

private val DIR_TTL = TimeStamp.fromDays(1)

class CachingCatalog @VisibleForTesting constructor(
    private val remote: RemoteCatalog,
    private val db: FileTree,
    id: String
) : Catalog {
    private val executor: CommandExecutor = CommandExecutor(id)

    constructor(ctx: Context, remote: RemoteCatalog, id: String) : this(
        remote,
        FileTree(ctx, id),
        id
    )

    override fun parseDir(path: Path, visitor: DirVisitor) {
        val dirName = path.getLocalId()
        executor.executeQuery("dir", object : QueryCommand {
            private val lifetime = db.getDirLifetime(dirName, DIR_TTL)
            override val isCacheExpired: Boolean
                get() = lifetime.isExpired

            override fun updateCache() {
                val entries = ArrayList<FileTree.Entry>(100)
                remote.parseDir(path, object : DirVisitor {
                    override fun acceptDir(name: String, description: String) {
                        entries.add(FileTree.Entry(name, description, null))
                    }

                    override fun acceptFile(name: String, description: String, size: String) {
                        entries.add(FileTree.Entry(name, description, size))
                    }
                })
                db.runInTransaction {
                    db.add(dirName, entries)
                    lifetime.update()
                }
            }

            override fun queryFromCache() = db.find(dirName)?.run {
                forEach {
                    // TODO: use extracted FileTree.Entry.feed
                    if (it.isDir) {
                        visitor.acceptDir(it.name, it.descr)
                    } else {
                        visitor.acceptFile(it.name, it.descr, it.size)
                    }
                }
                true
            } ?: false
        })
    }
}
