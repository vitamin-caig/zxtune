package app.zxtune.fs.httpdir

import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

open class RemoteCatalogTestBase {

    protected lateinit var catalog: RemoteCatalog

    internal enum class Mode {
        CHECK_EXISTING,
        CHECK_MISSED,
        CHECK_ALL
    }

    protected fun setUpTest(factory: (MultisourceHttpProvider) -> RemoteCatalog = ::RemoteCatalog) {
        catalog = factory(MultisourceHttpProvider(HttpProviderFactory.createTestProvider()))
    }

    protected fun test(path: Path, entries: Array<String>) =
        test(path, entries, Mode.CHECK_ALL)

    internal fun test(path: Path, entries: Array<String>, mode: Mode) =
        with(CheckingVisitor(entries, mode)) {
            catalog.parseDir(path, this)
            check()
        }

    internal class CheckingVisitor(
        entries: Array<String>,
        private val mode: Mode
    ) : Catalog.DirVisitor {

        private val etalon = HashMap<String, String>()

        init {
            for (idx in entries.indices step 2) {
                etalon[entries[idx]] = entries[idx + 1]
            }
        }

        override fun acceptDir(name: String, description: String) =
            acceptFile(name, description, "")

        override fun acceptFile(name: String, description: String, size: String) =
            test(name, size, description)

        private fun test(name: String, size: String, description: String) {
            val expected = etalon.remove(name)
            if (expected != null) {
                val expectedSizeAndDescr = expected.split("@")
                if (expectedSizeAndDescr.size == 2) {
                    assertEquals("Invalid size", expectedSizeAndDescr[0], size)
                    assertEquals("Invalid description", expectedSizeAndDescr[1], description)
                } else {
                    assertEquals("Invalid size", expected, size)
                }
            } else if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_EXISTING) {
                assertNotNull("Unexpected entry '${name}' $size", expected)
            }
        }

        fun check() {
            if (mode == Mode.CHECK_ALL || mode == Mode.CHECK_MISSED) {
                for ((name, size) in etalon) {
                    fail("Missed entry '${name}' $size")
                }
            }
        }
    }
}

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogParserTest {
    @Test
    fun `test xml index`() {
        val data = """<?xml version="1.0"?>
<list>
<directory mtime="2016-09-13T22:05:29Z">!MDScene_Arcade_VGM</directory>
<directory mtime="2018-06-27T07:25:08Z">x1</directory>
<file mtime="2018-06-27T06:24:42Z" size="1578655">hoot_2018-06-26.7z</file>
</list>"""
        val entries = arrayOf(
            "!MDScene_Arcade_VGM", "@2016-09-13T22:05:29Z",
            "x1", "@2018-06-27T07:25:08Z",
            "hoot_2018-06-26.7z", "1.5M@2018-06-27T06:24:42Z",
        )
        test(data, entries, RemoteCatalogTestBase.Mode.CHECK_ALL)
    }

    @Test
    fun `test table index`() {
        val data = """<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html>
 <head>
  <title>Index of /some/page</title>
 </head>
 <body>
<h1>Index of /some/page</h1>
  <table>
   <tr><th valign="top"><img src="/icons/blank.gif" alt="[ICO]"></th><th><a href="?C=N;O=D">Name</a></th><th><a href="?C=M;O=A">Last modified</a></th><th><a href="?C=S;O=A">Size</a></th><th><a href="?C=D;O=A">Description</a></th></tr>
   <tr><th colspan="5"><hr></th></tr>
<tr><td valign="top"><img src="/icons/back.gif" alt="[PARENTDIR]"></td><td><a href="/ayon/">Parent Directory</a>       </td><td>&nbsp;</td><td align="right">  - </td><td>&nbsp;</td></tr>
<tr><td valign="top"><img src="/icons/folder.gif" alt="[DIR]"></td><td><a href="scene_cpc/">scene_cpc/</a>             </td><td align="right">2018-02-22 00:31  </td><td align="right">  - </td><td>&nbsp;</td></tr>
<tr><td valign="top"><img src="/icons/unknown.gif" alt="[   ]"></td><td><a href="AT.ay">AT.ay</a>                  </td><td align="right">2018-04-25 03:13  </td><td align="right">7.1K</td><td>&nbsp;</td></tr>
   <tr><th colspan="5"><hr></th></tr>
</table>
</body></html>
"""
        val entries = arrayOf(
            "scene_cpc", "@2018-02-22 00:31",
            "AT.ay", "7.1K@2018-04-25 03:13",
        )
        test(data, entries, RemoteCatalogTestBase.Mode.CHECK_ALL)
    }

    // scene.org
    @Test
    fun `test table index 2`() {
        val data = """<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Index of /pub/music/groups/2063music/</title>
<style type="text/css">
a, a:active {text-decoration: none; color: blue;}
a:visited {color: #48468F;}
a:hover, a:focus {text-decoration: underline; color: red;}
body {background-color: #F5F5F5;}
h2 {margin-bottom: 12px;}
table {margin-left: 12px;}
th, td { font: 90% monospace; text-align: left;}
th { font-weight: bold; padding-right: 14px; padding-bottom: 3px;}
td {padding-right: 14px;}
td.s, th.s {text-align: right;}
div.list { background-color: white; border-top: 1px solid #646464; border-bottom: 1px solid #646464; padding-top: 10px; padding-bottom: 14px;}
div.foot { font: 90% monospace; color: #787878; padding-top: 4px;}
</style>
</head>
<body>
<h2>Index of /pub/music/groups/2063music/</h2>
<div class="list">
<table summary="Directory Listing" cellpadding="0" cellspacing="0">
<thead><tr><th class="n">Name</th><th class="m">Last Modified</th><th class="s">Size</th><th class="t">Type</th></tr></thead>
<tbody>
<tr class="d"><td class="n"><a href="../">..</a>/</td><td class="m">&nbsp;</td><td class="s">- &nbsp;</td><td class="t">Directory</td></tr>
<tr class="d"><td class="n"><a href="020200/">020200</a>/</td><td class="m">2002-Jul-10 19:36:52</td><td class="s">- &nbsp;</td><td class="t">Directory</td></tr>
<tr class="d"><td class="n"><a href="activities/">activities</a>/</td><td class="m">2004-Jun-10 13:03:57</td><td class="s">- &nbsp;</td><td class="t">Directory</td></tr>
<tr><td class="n"><a href="2063music.txt">2063music.txt</a></td><td class="m">2004-Jun-09 14:19:52</td><td class="s">0.6K</td><td class="t">text/plain</td></tr>
<tr><td class="n"><a href="63_001-opal2000-gatev0-5.mp3">63_001-opal2000-gatev0-5.mp3</a></td><td class="m">2000-Jun-22 03:23:54</td><td class="s">5.1M</td><td class="t">audio/mpeg</td></tr>
<tr><td class="n"><a href="63_002-opal2000-home_office.mp3">63_002-opal2000-home_office.mp3</a></td><td class="m">2000-Nov-10 01:38:00</td><td class="s">4.8M</td><td class="t">audio/mpeg</td></tr></tbody>
</table>
</div></body>
</html>"""
        val entries = arrayOf(
            "020200", "@2002-Jul-10 19:36:52",
            "activities", "@2004-Jun-10 13:03:57",
            "2063music.txt", "0.6K@2004-Jun-09 14:19:52",
            "63_001-opal2000-gatev0-5.mp3", "5.1M@2000-Jun-22 03:23:54",
            "63_002-opal2000-home_office.mp3", "4.8M@2000-Nov-10 01:38:00",
        )
        test(data, entries, RemoteCatalogTestBase.Mode.CHECK_ALL)
    }

    @Test
    fun `test PRE index`() {
        val data = """<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html>
 <head>
  <title>Index of /</title>
 </head>
 <body>
<h1>Index of /</h1>
<pre><img src="/icons/blank.gif" alt="Icon "> <a href="?C=N;O=D">Name</a>                    <a href="?C=M;O=A">Last modified</a>      <a href="?C=S;O=A">Size</a>  <a href="?C=D;O=A">Description</a><hr><img src="/icons/folder.gif" alt="[DIR]"> <a href="!MDScene_Arcade_VGM/">!MDScene_Arcade_VGM/</a>    2016-09-13 18:05    -   
<img src="/icons/unknown.gif" alt="[   ]"> <a href="hoot_2018-06-26.7z">hoot_2018-06-26.7z</a>      2018-06-27 02:24  1.5M  <span class='description'>7-Zip Archive</span>
<img src="/icons/folder.gif" alt="[DIR]"> <a href="x1/">x1/</a>                     2018-06-27 03:25    -   
<hr></pre>
</body></html>
"""
        val entries = arrayOf(
            "!MDScene_Arcade_VGM", "@2016-09-13 18:05",
            "x1", "@2018-06-27 03:25",
            "hoot_2018-06-26.7z", "1.5M@2018-06-27 02:24",
        )
        test(data, entries, RemoteCatalogTestBase.Mode.CHECK_ALL)
    }

    private fun test(data: String, entries: Array<String>, mode: RemoteCatalogTestBase.Mode) =
        with(RemoteCatalogTestBase.CheckingVisitor(entries, mode)) {
            RemoteCatalog.parseDir(data.byteInputStream(), this)
            check()
        }
}