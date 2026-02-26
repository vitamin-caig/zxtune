package app.zxtune.fs.vgmrips

/**
 * Defines any grouping entity (chip, company, composer, system etc)
 */
data class Group(val id: Id, val title: String, val packs: Int, val image: FilePath? = null) {
    @JvmInline
    value class Id(val value: String)
}
