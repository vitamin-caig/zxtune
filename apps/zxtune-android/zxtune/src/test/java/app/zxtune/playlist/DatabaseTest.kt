package app.zxtune.playlist

import androidx.core.testing.util.TestConsumer
import androidx.room.Room
import androidx.room.migration.Migration
import androidx.test.core.app.ApplicationProvider
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class DatabaseTest {

    private lateinit var underTest: Database

    @Before
    fun setUp() {
        underTest = Database(
            Room.inMemoryDatabaseBuilder(
                ApplicationProvider.getApplicationContext(), DatabaseDelegate::class.java
            ).allowMainThreadQueries().build()
        )
    }

    @After
    fun tearDown() {
        underTest.close()
    }

    @Test
    fun `migrate from v1`() =
        with(Database(makeDatabaseFromResource("playlist_v1.db", Migration1to3))) {
            TestConsumer<Playlist>().apply {
                queryPlaylists(this)
            }.assertValues(emptyList())
            getPlaylist().verifyTracksAre(
                makeTrack(
                    1,
                    "zxart:/Top/nq%20-%20synchronization%20(2015).pt3?track=60891",
                    "synchronization",
                    "nq/skrju  27.06.2015",
                    501760,
                ),
                makeTrack(
                    3,
                    "zxart:/Top/Pator%20-%20Digital%20Espresso%20(2023)%20(Revision%202023%2C%2012).pt3?track=409532",
                    "digital espresso 4revision2023.:",
                    "pator/Joker^PL.ZXAllStars AY ABC",
                    171380,
                ),
                makeTrack(
                    122,
                    "zxart:/Top/Pator%20-%20Last%20Minute%20Accident%20(2023)%20(speccy.pl%20party%202023.1%2C%201).pt3?track=416272",
                    "last minute accident",
                    "pator/joker&speccy.pl ay abc",
                    124480
                ),
            )
            close()
        }

    @Test
    fun `migrate from v2`() =
        with(Database(makeDatabaseFromResource("playlist_v2.db", Migration2to3))) {
            TestConsumer<Playlist>().apply {
                queryPlaylists(this)
            }.assertValues(emptyList())
            getPlaylist().verifyTracksAre(
                makeTrack(
                    120,
                    "zxart:/Top/Mister%20BEEP%20-%20Nebula%20Fight%20(2010)%20(DiHalt%202010%2C%201).ay?track=47899",
                    "Nebula Fight",
                    "Mister Beep",
                    133000,
                ),
                makeTrack(
                    5,
                    "zxart:/Top/Mic%20-%20Dreamless%20(1997)%20(Enlight%201997).stp?track=44159",
                    "'DREAMLESS' BY MIC/PGS!!!",
                    "",
                    158720,
                ),
                makeTrack(
                    2,
                    "zxart:/Top/MmcM%20-%20AsSuRed%20(2000).pt3?track=74340",
                    "AsSuRed ... Hi! My Frends ...",
                    "Mm<M of Sage 14.Apr.XX twr 00:37",
                    221340,
                ),
            )
            close()
        }

    @Test
    fun `add and query`() = with(underTest) {
        val allTracks = mutableListOf<Track>()
        val playlists = Array(3) { idx ->
            val id = if (idx > 0) addPlaylist("playlist$idx") else Playlist.DEFAULT_ID
            getPlaylist(id).apply {
                repeat(idx + 1) {
                    val meta = makeTrack(allTracks.size)
                    val id = addTrack(meta)
                    assert(id.value > 0)
                    allTracks.add(Track(id, meta))
                }
            }
        }
        TestConsumer<Playlist>().apply {
            queryPlaylists(this)
        }.assertValues(
            listOf(
                Playlist(Playlist.Id(1), "playlist1"), Playlist(Playlist.Id(2), "playlist2")
            )
        )

        // 1
        // 2 3
        // 4 5 6
        playlists[0].verifyTracksAre(allTracks[0])
        playlists[1].verifyTracksAre(allTracks[1], allTracks[2])
        playlists[2].verifyTracksAre(allTracks[3], allTracks[4], allTracks[5])
        TestConsumer<Track>().apply {
            queryTracks(Track.IdSet.of(0, 3, 1, 6, 100500), this)
        }.assertValues(listOf(allTracks[0], allTracks[2], allTracks[5]))
    }

    @Test
    fun `duplicates policy`() = with(underTest) {
        val originalMeta = makeTrack(0)
        val playlist = getPlaylist()
        val originalId = playlist.addTrack(track = originalMeta)
        val unchangedMeta = makeTrack(1)
        val unchangedId = playlist.addTrack(track = unchangedMeta)
        playlist.verifyTracksAre(
            Track(originalId, originalMeta),
            Track(unchangedId, unchangedMeta),
        )
        val updatedMeta = originalMeta.copy(title = "Updated title")
        val updatedId = playlist.addTrack(track = updatedMeta)
        playlist.verifyTracksAre(
            Track(originalId, originalMeta),
            Track(unchangedId, unchangedMeta),
            Track(updatedId, updatedMeta),
        )
    }

    @Test
    fun statistics() = with(underTest) {
        val playlist1 = getPlaylist()
        val duplicatedTrack1 = makeTrack(1)
        val ids1 = arrayOf(
            playlist1.addTrack(makeTrack(2)),
            playlist1.addTrack(duplicatedTrack1),
            playlist1.addTrack(makeTrack(3)),
            playlist1.addTrack(duplicatedTrack1),
        )
        val playlist2 = addPlaylist("new playlist").let {
            getPlaylist(it)
        }
        val duplicatedTrack2 = makeTrack(4)
        val ids2 = playlist2.run {
            arrayOf(
                addTrack(duplicatedTrack2),
                addTrack(makeTrack(5)),
                addTrack(duplicatedTrack2),
                addTrack(makeTrack(6)),
                addTrack(duplicatedTrack1),
            )
        }
        playlist1.queryStatistics().run {
            assertEquals(4, count)
            assertEquals(3, locations)
            assertEquals(2 + 1 + 3 + 1, duration.toSeconds())
        }
        playlist2.queryStatistics().run {
            assertEquals(5, count)
            assertEquals(4, locations)
            assertEquals(4 + 5 + 4 + 6 + 1, duration.toSeconds())
        }
        queryStatistics(Track.IdSet.of(ids1[1], ids1[2], ids2[0], ids2[2], ids2[4])).run {
            assertEquals(5, count)
            assertEquals(3, locations)
            assertEquals(1 + 3 + 4 + 4 + 1, duration.toSeconds())
        }
    }

    @Test
    fun deleting() = with(underTest) {
        val meta = makeTrack(0)
        val playlist1 = getPlaylist()
        val tracks1 = Track.IdSet(3) {
            playlist1.addTrack(meta)
        }
        val playlist2 = addPlaylist("title").let {
            getPlaylist(it)
        }
        val tracks2 = Array(4) {
            playlist2.addTrack(meta)
        }
        val shared = Track.IdSet(2) {
            playlist1.addTrack(meta)
        }
        playlist2.addTracks(shared)

        playlist1.run {
            assert(exists())
            verifyTracksAre(tracks1[0], tracks1[1], tracks1[2], shared[0], shared[1])
            deleteTracks(Track.IdSet.of(tracks1[1], shared[0]))
            verifyTracksAre(tracks1[0], tracks1[2], shared[1])
            delete()

            assert(exists())
            verifyNoTracks()
        }

        playlist2.run {
            assert(exists())
            verifyTracksAre(tracks2[0], tracks2[1], tracks2[2], tracks2[3], shared[0], shared[1])
            delete()

            assert(!exists())
            verifyNoTracks()
        }
        TestConsumer<Playlist>().apply {
            queryPlaylists(this)
        }.assertValues(emptyList())
    }

    @Test
    fun sorting() = with(underTest) {
        val location = makeLocation(0)
        val playlist = getPlaylist()
        // 1324/4231 2341/1432 3142/2413
        val t1 = playlist.addTrack(Track.Metadata(location, "A", "d", TimeStamp.fromSeconds(6)))
        val t2 = playlist.addTrack(Track.Metadata(location, "C", "a", TimeStamp.fromSeconds(8)))
        val t3 = playlist.addTrack(Track.Metadata(location, "B", "b", TimeStamp.fromSeconds(5)))
        val t4 = playlist.addTrack(Track.Metadata(location, "D", "c", TimeStamp.fromSeconds(7)))
        val playlist2 = addPlaylist("new playlist").let {
            getPlaylist(it)
        }.apply {
            addTracks(Track.IdSet.of(t1, t2, t3, t4))
        }

        val initial = arrayOf(t1, t2, t3, t4)
        val titleSorted = arrayOf(t1, t3, t2, t4)
        val authorSorted = arrayOf(t2, t3, t4, t1)
        val durationSorted = arrayOf(t3, t1, t4, t2)

        var p1 = initial
        var p2 = initial

        for ((field, expected) in arrayOf(
            Playlist.Sorting.By.TITLE to titleSorted,
            Playlist.Sorting.By.AUTHOR to authorSorted,
            Playlist.Sorting.By.DURATION to durationSorted
        )) {
            playlist.run {
                verifyTracksAre(*p1)
                sort(Playlist.Sorting(field, Playlist.Sorting.Order.ASCENDING))
                p1 = expected
                verifyTracksAre(*p1)
            }
            playlist2.run {
                verifyTracksAre(*p2)
                sort(Playlist.Sorting(field, Playlist.Sorting.Order.DESCENDING))
                p2 = expected.reversed().toTypedArray()
                verifyTracksAre(*p2)
            }
        }
    }

    @Test
    fun reordering() = with(underTest) {
        val playlists = Array(4) {
            val id = if (it == 0) {
                Playlist.DEFAULT_ID
            } else {
                addPlaylist("p$it")
            }
            getPlaylist(id)
        }
        val tracks = Array(13) { idx ->
            playlists[0].addTrack(track = makeTrack(idx)).also { trackId ->
                playlists.indices.shuffled().filter { it != 0 && 0 == idx % (it + 1) }
                    .map { playlists[it] }.forEach { playlist ->
                        playlist.addTracks(Track.IdSet.of(trackId))
                    }
            }
        }
        playlists[1].verifyTracksAre(
            tracks[0],
            tracks[2],
            tracks[4],
            tracks[6],
            tracks[8],
            tracks[10],
            tracks[12],
        )
        playlists[2].verifyTracksAre(tracks[0], tracks[3], tracks[6], tracks[9], tracks[12])
        playlists[3].verifyTracksAre(tracks[0], tracks[4], tracks[8], tracks[12])

        // across
        playlists[1].run {
            reorder(tracks[4], 3)  // 0 2 6 8 10 4 12
            reorder(tracks[8], -2) // 0 8 2 6 10 4 12
        }

        // across, out of bounds
        playlists[2].run {
            reorder(tracks[0], 10)
            reorder(tracks[12], -10)
        }

        // out of bounds
        playlists[3].run {
            reorder(tracks[0], -5)
            reorder(tracks[12], 5)
        }

        // nonexisting
        playlists[1].reorder(tracks[9], 7)

        playlists[1].verifyTracksAre(
            tracks[0], tracks[8], tracks[2], tracks[6], tracks[10], tracks[4], tracks[12]
        )
        playlists[2].verifyTracksAre(tracks[12], tracks[3], tracks[6], tracks[9], tracks[0])
        playlists[3].verifyTracksAre(tracks[0], tracks[4], tracks[8], tracks[12])
    }

    @Test
    fun migrating() = with(underTest) {
        val source = getPlaylist()
        val targetId = addPlaylist("target")
        val target = getPlaylist(targetId)
        val meta = makeTrack(0)
        val migrating1 = source.addTrack(meta)
        val uniq1 = source.addTrack(meta)
        val uniq2 = target.addTrack(meta)
        val migrating2 = source.addTrack(meta)
        val shared = source.addTrack(meta).also {
            target.addTracks(Track.IdSet.of(it))
        }

        source.verifyTracksAre(migrating1, uniq1, migrating2, shared)
        target.verifyTracksAre(uniq2, shared)

        source.moveTracks(Track.IdSet.of(migrating1, migrating2), targetId)

        source.verifyTracksAre(uniq1, shared)
        target.verifyTracksAre(uniq2, shared, migrating1, migrating2)
    }
}

private fun Database.PlaylistFacade.verifyNoTracks() = TestConsumer<Track>().apply {
    queryTracks(this)
}.assertValues(emptyList())

private fun Database.PlaylistFacade.verifyTracksAre(
    vararg tracks: Track
) = TestConsumer<Track>().apply {
    queryTracks(this)
}.assertValues(tracks.toList())

private fun Database.PlaylistFacade.verifyTracksAre(
    vararg tracks: Track.Id
) = TestConsumer<Track.Id>().apply {
    queryTracks() {
        accept(it.id)
    }
}.assertValues(tracks.toList())

private fun makeDatabaseFromResource(name: String, migration: Migration) = Room.databaseBuilder(
    ApplicationProvider.getApplicationContext(), DatabaseDelegate::class.java, Database.NAME
).allowMainThreadQueries().addMigrations(migration).createFromInputStream {
    DatabaseTest::class.java.classLoader!!.getResourceAsStream("databases/$name")
}.build()

private fun makeTrack(id: Int) = Track.Metadata(
    location = makeLocation(id),
    title = "Track $id",
    author = "By $id",
    duration = TimeStamp.fromSeconds(id.toLong())
)

private fun makeTrack(id: Long, location: String, title: String, author: String, duration: Int) =
    Track(
        Track.Id(id), Track.Metadata(
            Identifier.parse(location), title, author, TimeStamp.fromMilliseconds(
                duration.toLong()
            )
        )
    )

private fun makeLocation(id: Int) = Identifier.parse("scheme://host/path/$id")
