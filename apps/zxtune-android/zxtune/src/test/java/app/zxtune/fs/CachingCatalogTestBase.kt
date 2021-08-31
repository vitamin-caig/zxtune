package app.zxtune.fs

import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.utils.ProgressCallback
import org.junit.Assert
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.mock
import java.io.IOException

open class CachingCatalogTestBase(protected val case: TestCase) {
    enum class TestCase(
        val isFailedRemote: Boolean,
        val hasCache: Boolean,
        val isCacheExpired: Boolean
    ) {
        `empty up-to-date cache`(false, false, false),
        `fill empty cache`(false, false, true),
        `take from up-to-date cache`(false, true, false),
        `update expired cache`(false, true, true),
        `empty up-to-date cache with failed remote`(true, false, false),
        `failed to fill empty cache`(true, false, true),
        `take from up-to-date cache with failed remote`(true, true, false),
        `failed to update, take from expired cache`(true, true, true)
    }

    protected val lifetime = mock<Timestamps.Lifetime> {
        on { isExpired } doReturn case.isCacheExpired
    }

    protected val progress = mock<ProgressCallback>()

    protected val error = IOException()

    protected fun checkedQuery(cmd: () -> Unit) {
        try {
            cmd()
            assert(!case.isFailedRemote || !case.isCacheExpired || case.hasCache)
        } catch (e: IOException) {
            Assert.assertSame(e, error)
        }
    }
}