package app.zxtune.ui.utils

private class ImmutableList<T>(private val inner: List<T>) : List<T> by inner

private fun <T> List<T>.toImmutable(): List<T> = (this as? ImmutableList<T>) ?: ImmutableList(this)

class FilteredListState<T> private constructor(
    private val fullEntries: List<T>,
    val filter: String = "",
    filtered: List<T>? = null,
    private val match: (T, String) -> Boolean,
) {
    constructor(match: (T, String) -> Boolean) : this(fullEntries = emptyList(), match = match)

    private fun clone(
        newEntries: List<T> = fullEntries,
        newFilter: String = filter,
        newFiltered: List<T>? = filteredEntries
    ) = FilteredListState(newEntries, newFilter, newFiltered, match)

    private val filteredEntries = filtered?.toImmutable()

    val entries
        get() = filteredEntries ?: fullEntries

    fun withContent(newContent: List<T>) = when {
        filter.isEmpty() -> FilteredListState(fullEntries = newContent, match = match)
        newContent.isEmpty() -> clone(newEntries = newContent, newFiltered = newContent)
        else -> clone(
            newEntries = newContent,
            newFiltered = newContent.filter { match(it, filter) }
        )
    }

    fun withFilter(newFilter: String) = when {
        newFilter.isBlank() || fullEntries.isEmpty() -> FilteredListState(
            fullEntries = fullEntries,
            match = match,
        )
        newFilter == filter -> this
        filter.isNotEmpty() && newFilter.startsWith(filter) -> clone(
            newFilter = newFilter,
            newFiltered = filteredEntries?.filter { match(it, newFilter) },
        )
        else -> clone(
            newFilter = newFilter,
            newFiltered = fullEntries.filter { match(it, newFilter) },
        )
    }
}
