package app.zxtune

// TODO: migrate to JUnit after upgrade
internal inline fun <reified T> assertThrows(cmd: () -> Unit): T = try {
    cmd()
    fail("${T::class.java.name} was not thrown")
} catch (e: Throwable) {
    when (e) {
        is AssertionError -> throw e
        is T -> e
        else -> fail("Unexpected exception ${e::class.java.name}: ${e.message}")
    }
}

fun fail(msg: String): Nothing = throw AssertionError(msg)
