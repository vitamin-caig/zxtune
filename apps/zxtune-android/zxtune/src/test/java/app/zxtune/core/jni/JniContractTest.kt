package app.zxtune.core.jni

import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

/**
 * A missing keep is only observable as a crash inside forcedInit on a device, so
 * the required set is derived from the native sources rather than restated here
 */
class JniContractTest {

    @Test
    fun `classes reached by FindClass are kept`() = assertKept(findClassNames)

    @Test
    fun `classes named by a JNI descriptor are kept`() = assertKept(descriptorNames)

    @Test
    fun `classes exporting a JNI symbol are kept`() = assertKept(exportNames)

    @Test
    fun `proguard rules reference existing classes`() = assertEquals(
        "rules naming classes that no longer exist, remove them",
        emptyList<String>(),
        proguardClasses.filter { it.startsWith("$APP_PACKAGE.") }
            .filter { it.substringBefore('$') !in declarations },
    )

    private fun assertKept(names: Set<String>) = assertEquals(
        "not kept for JNI: add @Keep to the declaration or a rule to proguard-rules.pro",
        emptyList<String>(),
        names.filterNot(::isKept),
    )

    private val jniSources by lazy {
        jniDir.walkTopDown().filter { it.extension in setOf("cpp", "h") && it.isFile }
            .map { it.readText() }.toList()
    }

    // FindClass arguments
    private val findClassNames by lazy {
        jniSources.flatMap { text -> FIND_CLASS.findAll(text).map { it.groupValues[1] } }
            .map { it.replace('/', '.') }.toSet()
    }

    // Object types referenced from JNI method descriptors
    private val descriptorNames by lazy {
        jniSources.flatMap { text -> DESCRIPTOR.findAll(text).map { it.groupValues[1] } }
            .map { it.replace('/', '.') }.toSet()
    }

    // The class is the longest run of tokens resolving to a real declaration, which
    // sidesteps having to know where the method name starts
    private val exportNames by lazy {
        jniSources.flatMap { text -> EXPORT.findAll(text).map { it.value } }.map { symbol ->
            var prefix = ""
            var resolved: String? = null
            for (token in symbol.removePrefix("Java_").split('_')) {
                assertFalse("unsupported '_' escaping in $symbol", token == "_1")
                prefix = if (prefix.isEmpty()) token else "$prefix.$token"
                if (prefix in declarations) resolved = prefix
            }
            requireNotNull(resolved) { "no declaration for the class of $symbol" }
        }.toSet()
    }

    private fun isKept(fqcn: String) = hasKeepOn(fqcn) || proguardCovers(fqcn) || exportsNativeMethod(fqcn)

    /**
     * Scans the annotation block directly above the declaration rather than
     * grepping the file, so a file where only one of several types is annotated
     * cannot pass for all of them
     */
    private fun hasKeepOn(fqcn: String): Boolean {
        val file = declarations[fqcn.substringBefore('$')] ?: return false
        val name = fqcn.substringAfterLast('.').substringAfterLast('$')
        val lines = file.readLines()
        val at = lines.indexOfFirst { DECLARED.containsMatchIn(it) && word(name, it) }
        if (at < 0) return false
        return lines.subList(0, at).takeLastWhile { it.trimStart().startsWith("@") }
            .any { it.trim() == "@Keep" }
    }

    private fun word(name: String, line: String) = Regex("\\b$name\\b").containsMatchIn(line)

    /**
     * R8's built-in rule
     * -keepclasseswithmembernames class * { native <methods>; }
     * pins class names that export native methods, so classes like JniApi that
     * are reached through their JNI symbol alone need no explicit keep
     */
    private fun exportsNativeMethod(fqcn: String): Boolean {
        val file = declarations[fqcn.substringBefore('$')] ?: return false
        val name = fqcn.substringAfterLast('.').substringAfterLast('$')
        val lines = file.readLines()
        val at = lines.indexOfFirst { DECLARED.containsMatchIn(it) && word(name, it) }
        if (at < 0) return false
        val indent = lines[at].indentLength()
        return lines.subList(at + 1, lines.size)
            .takeWhile { it.isBlank() || it.indentLength() > indent }
            .any { "external" in it || "native" in it }
    }

    private fun String.indentLength() = takeWhile { it.isWhitespace() }.length

    private val proguardClasses by lazy {
        KEEP_RULE.findAll(proguardRules.readText()).map { it.groupValues[1] }.toSet()
    }

    private fun proguardCovers(fqcn: String) = proguardClasses.any { pattern ->
        if (pattern.all { it == '*' }) {
            // the Room rule's bare `*` is filtered by `extends` and does not match anything real
            return@any false
        }
        Regex(pattern.split("*").joinToString(".*") { Regex.escape(it) }).matches(fqcn)
    }

    // Top-level declarations only, which is all the name lookups above need
    private val declarations by lazy {
        javaDir.walkTopDown().filter { it.extension in setOf("kt", "java") && it.isFile }
            .flatMap { file ->
                val text = file.readText()
                val pkg = PACKAGE.find(text)?.groupValues?.get(1) ?: return@flatMap emptySequence()
                TOP_LEVEL.findAll(text).map { it.groupValues[1] }
                    .map { if (pkg.isEmpty()) it else "$pkg.$it" }.map { it to file }
            }.toMap()
    }

    private val moduleDir by lazy {
        generateSequence(File("").absoluteFile) { it.parentFile }
            .firstOrNull { it.resolve("proguard-rules.pro").isFile }
            ?: error("cannot locate the module directory from ${File("").absolutePath}")
    }

    private val javaDir by lazy { moduleDir.resolve("src/main/java") }

    private val jniDir by lazy { moduleDir.resolve("src/main/jni") }

    private val proguardRules by lazy { moduleDir.resolve("proguard-rules.pro") }

    companion object {
        private const val APP_PACKAGE = "app.zxtune"

        // The package is interpolated into patterns, so it is escaped. The native
        // sources spell it with '/' in FindClass arguments and descriptors, and with
        // '_' in exported symbols, neither of which is the dotted form.
        private val APP_SLASH = Regex.escape(APP_PACKAGE.replace('.', '/'))
        private val APP_UNDERSCORE = Regex.escape(APP_PACKAGE.replace('.', '_'))

        private val FIND_CLASS = Regex("\"($APP_SLASH[A-Za-z0-9_/$]*)\"")
        private val DESCRIPTOR = Regex("L($APP_SLASH[A-Za-z0-9_/$]*);")
        private val EXPORT = Regex("Java_$APP_UNDERSCORE[A-Za-z0-9_]+")
        private val KEEP_RULE = Regex(
            "^-(?:keep|keepnames)\\s+(?:\\w+\\s+)*class\\s+([\\w.$*]+)",
            RegexOption.MULTILINE,
        )
        private val PACKAGE = Regex("^package\\s+([\\w.]+)", RegexOption.MULTILINE)
        private val TOP_LEVEL = Regex(
            "^(?:\\w+\\s+)*(?:class|interface|object)\\s+(\\w+)",
            RegexOption.MULTILINE,
        )

        private val DECLARED = Regex("^\\s*(?:\\w+\\s+)*(?:class|interface|object)\\s+\\w+")
    }
}
