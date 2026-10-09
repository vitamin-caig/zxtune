#!/usr/bin/env python3
"""Tests for make/android/r8report.py.

The mapping fixtures below are hand-reduced from a real fatMinsdk16Release
mapping.txt, keeping the shapes that are easy to get wrong: a multi-frame group
whose inlined frames precede the retained one, a frame that names only a method
(so the owner has to come from the enclosing class block), a residual-signature
comment landing in the middle of a group, and a symbol that is retained at one
call site while inlined at another.
"""

import contextlib
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import r8report  # noqa: E402

MAPPING = """\
# compiler: R8
# compiler_version: 9.4.14
# min_api: 16
app.zxtune.core.jni.JniGC -> g20:
# {"id":"sourceFile","fileName":"JniGC.kt"}
    java.util.Set refs -> b
    1:7:void <clinit>():13:13 -> <clinit>
    8:14:void <clinit>():14 -> <clinit>
app.zxtune.core.jni.JniGC$1 -> e20:
# {"id":"sourceFile","fileName":"JniGC.kt"}
    1:3:java.lang.Object invokeSuspend(java.lang.Object):18:18 -> p
    4:14:boolean app.zxtune.core.jni.Plugins.canLoad():5:5 -> p
    4:14:java.lang.Object app.zxtune.core.jni.JniGC.cleanup():26:26 -> p
    4:14:java.lang.Object invokeSuspend(java.lang.Object):20 -> p
    20:30:boolean app.zxtune.core.jni.JniGC$HandleReference.destroy():48:48 -> p
      # {"id":"com.android.tools.r8.residualsignature","signature":"()Z"}
    20:30:java.lang.Object app.zxtune.core.jni.JniGC.cleanup():28 -> p
    20:30:java.lang.Object invokeSuspend(java.lang.Object):20 -> p
    31:34:boolean app.zxtune.core.jni.JniGC$HandleReference.destroy():49:49 -> p
    31:34:java.lang.Object invokeSuspend(java.lang.Object):20 -> p
    40:41:void app.zxtune.core.jni.JniGC$HandleReference.destroy():52:52 -> q
app.zxtune.core.jni.JniGC$HandleReference -> f20:
    4:6:void <init>(java.lang.Object):44:44 -> <init>
      # {"id":"com.android.tools.r8.residualsignature","signature":"(Ljava/lang/Object;)V"}
    1:2:boolean app.zxtune.core.jni.Plugins.canTrigger():0:0 -> t
      # {"id":"com.android.tools.r8.synthesized"}
    1:3:boolean app.zxtune.core.jni.Plugins.canDetect():0:0 -> u
"""

SEEDS = """\
app.zxtune.core.jni.Plugins
app.zxtune.core.jni.JniGC
app.zxtune.core.jni.JniGC$HandleReference
"""

USAGE = """\
android.support.v4.app.INotificationSideChannel:
    public java.lang.String getInterfaceDescriptor()
app.zxtune.core.jni.JniGC$HandleReference:
    public final boolean destroy()
    public void <init>(java.lang.Object)
    java.lang.Object owner
app.zxtune.core.jni.JniGC:
    public void <init>()
    public final void cleanup()
    public static void register(java.lang.Object)
    int missing
app.zxtune.core.jni.Plugins
"""


def _write(tmp, name, text):
    path = os.path.join(tmp, name)
    with open(path, "w", encoding="utf-8") as dst:
        dst.write(text)
    return path


class ParseMappingTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.mapping = r8report.parse_mapping(_write(self.tmp, "mapping.txt", MAPPING))

    def test_classes_are_collected(self):
        self.assertEqual(
            {
                "app.zxtune.core.jni.JniGC",
                "app.zxtune.core.jni.JniGC$1",
                "app.zxtune.core.jni.JniGC$HandleReference",
            },
            self.mapping.classes,
        )

    def test_inlined_frames_exclude_the_last_of_each_group(self):
        # cleanup and destroy are folded into invokeSuspend
        self.assertIn("app.zxtune.core.jni.JniGC.cleanup", self.mapping.inlined)
        self.assertIn("app.zxtune.core.jni.JniGC$HandleReference.destroy", self.mapping.inlined)

    def test_last_frame_of_a_group_is_retained(self):
        self.assertIn("app.zxtune.core.jni.JniGC$1.invokeSuspend", self.mapping.retained)

    def test_residual_signature_comment_does_not_break_a_group(self):
        # the comment sits between the first and second frame of the 20:30 group;
        # if it split the group, the 20:30 destroy frame would read as retained
        self.assertIn("app.zxtune.core.jni.JniGC$HandleReference.destroy", self.mapping.inlined)
        self.assertIn("app.zxtune.core.jni.JniGC$HandleReference.destroy", self.mapping.retained)

    def test_inlined_members_are_indexed_by_owner(self):
        self.assertIn("canLoad", self.mapping.inlined_owners["app.zxtune.core.jni.Plugins"])

    def test_frame_without_owner_uses_the_enclosing_class(self):
        # invokeSuspend has no declaring class in the frame, the block supplies it
        self.assertIn("app.zxtune.core.jni.JniGC$1.invokeSuspend", self.mapping.retained)
        for sig in self.mapping.retained | self.mapping.inlined:
            self.assertIn(".", sig.split(".")[0] + ".", sig)

    def test_constructor_frame_keeps_its_owner(self):
        self.assertIn("app.zxtune.core.jni.JniGC$HandleReference.<init>", self.mapping.retained)
        self.assertIn("app.zxtune.core.jni.JniGC.<clinit>", self.mapping.retained)

    def test_fields_are_collected(self):
        self.assertIn("app.zxtune.core.jni.JniGC.refs", self.mapping.fields)

    def test_zero_origin_frames_are_ordinary_retained_members(self):
        # ':0:0' (no source line) is normal for compiler/R8-generated bodies,
        # including the synthesized-annotation variant; both are just retained
        self.assertIn("app.zxtune.core.jni.Plugins.canDetect", self.mapping.retained)
        self.assertIn("app.zxtune.core.jni.Plugins.canTrigger", self.mapping.retained)


class ClassifyTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        mapping = r8report.parse_mapping(_write(self.tmp, "mapping.txt", MAPPING))
        usage = _write(self.tmp, "usage.txt", USAGE)
        self.records = r8report.classify(mapping, usage, "app.zxtune")
        self.by_sig = {}
        for record in self.records:
            for member in record["members"]:
                self.by_sig[member.sig] = member.verdict

    def test_symbol_inlined_at_every_site(self):
        # cleanup is folded into invokeSuspend by two different groups and is
        # never kept as a method of its own
        self.assertEqual("INLINED", self.by_sig["app.zxtune.core.jni.JniGC.cleanup"])

    def test_retained_and_inlined_at_different_sites_is_reported_as_both(self):
        # the 40:41 group keeps destroy, the 20:30 and 31:34 groups inline it
        self.assertEqual("both", self.by_sig["app.zxtune.core.jni.JniGC$HandleReference.destroy"])

    def test_retained_constructor(self):
        self.assertEqual("RETAINED", self.by_sig["app.zxtune.core.jni.JniGC$HandleReference.<init>"])

    def test_constructor_absent_from_the_mapping_is_gone(self):
        # R8 lists no <init> frame for JniGC, its Kotlin constructor is implicit
        self.assertEqual("GONE", self.by_sig["app.zxtune.core.jni.JniGC.<init>"])

    def test_symbol_absent_from_the_mapping_is_gone(self):
        self.assertEqual("GONE", self.by_sig["app.zxtune.core.jni.JniGC.register"])

    def test_removed_field_is_gone(self):
        self.assertEqual("GONE", self.by_sig["app.zxtune.core.jni.JniGC.missing"])
        self.assertEqual("GONE", self.by_sig["app.zxtune.core.jni.JniGC$HandleReference.owner"])

    def test_removed_class_is_its_own_record(self):
        gone = [r for r in self.records if r["removed"] and r["class_gone"]]
        self.assertEqual(["app.zxtune.core.jni.Plugins"], [r["name"] for r in gone])

    def test_removed_class_keeps_its_inlined_members(self):
        plugins = next(r for r in self.records if r["name"] == "app.zxtune.core.jni.Plugins")
        self.assertEqual(("canLoad",), plugins["inlined_members"])

    def test_other_packages_are_excluded(self):
        for record in self.records:
            self.assertTrue(record["name"].startswith("app.zxtune"))

    def test_classification_covers_every_usage_member(self):
        # 3 members of JniGC$HandleReference plus 4 of JniGC
        self.assertEqual(7, len(self.by_sig))


class SummaryTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.mapping = r8report.parse_mapping(_write(self.tmp, "mapping.txt", MAPPING))
        usage = _write(self.tmp, "usage.txt", USAGE)
        self.records = r8report.classify(self.mapping, usage, "app.zxtune")
        self.info = r8report.summarize(
            self.records, "app.zxtune", "test", "usage", "mapping", self.mapping
        )

    def test_summary_lists_removed_classes_as_a_tree(self):
        summary = r8report.render_summary(self.info, self.records, set())
        self.assertIn("removed classes (1) + weakened classes (2):", summary)
        self.assertRegex(summary, r"`- app/ \(0\)[\s\S]*`- zxtune/ \(0\)[\s\S]*`- core/ \(0\)[\s\S]*`- jni/ \(3\)[\s\S]*|- JniGC \(removed: <init>, missing, register\)")
        self.assertRegex(summary, r"|- JniGC\$HandleReference \(removed: owner\)")
        self.assertRegex(summary, r"`- Plugins \(inlined: canLoad\)")

    def test_class_leaf_states_are_explicit(self):
        planted = [
            {
                "name": "app.zxtune.core.jni.Plugins",
                "removed": True,
                "inlined_members": ("canLoad",),
            },
            {
                "name": "app.zxtune.core.jni.JniOptions",
                "removed": True,
                "inlined_members": (),
            },
        ]
        summary = r8report.render_summary(self.info, planted, set())
        self.assertIn("`- Plugins (inlined: canLoad)", summary)
        self.assertIn("|- JniOptions (removed in whole)", summary)

    def test_removed_class_inlined_members_feed_the_inlined_count(self):
        # cleanup (a reported member) and canLoad (from a removed class) are both
        # folded into invokeSuspend; both must count as INLINED
        self.assertEqual(2, self.info["counts"]["INLINED"])

    def test_summary_never_mentions_standard_packages(self):
        planted = [
            {"name": "app.zxtune.Runner", "removed": True},
            {"name": "java.lang.Object", "removed": True},
            {"name": "android.app.Activity", "removed": True},
            {"name": "kotlin.Unit", "removed": True},
        ]
        summary = r8report.render_summary(self.info, planted, set())
        self.assertIn("removed classes (1) + weakened classes (0):", summary)
        self.assertRegex(summary, r"`- app/[\s\S]*`- zxtune/[\s\S]*`- Runner")
        for standard in ("java.lang.Object", "android.app.Activity", "kotlin.Unit"):
            self.assertNotIn(standard, summary)

    def test_weakened_classes_listed_by_name_and_members(self):
        self.assertEqual(
            [
                ("app.zxtune.core.jni.JniGC", ("<init>", "missing", "register")),
                ("app.zxtune.core.jni.JniGC$HandleReference", ("owner",)),
            ],
            r8report._weakened_classes(self.records),
        )


class LiveReportTest(unittest.TestCase):
    """Checks the real dumps, when a build has produced them."""

    ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
    RELEASE = os.path.join(ROOT, "pkg", "zxtune", "outputs", "mapping", "fatMinsdk16Release")

    def setUp(self):
        if not os.path.isfile(os.path.join(self.RELEASE, "usage.txt")):
            self.skipTest("no R8 output; run :zxtune:minifyFatMinsdk16ReleaseWithR8 first")

    def test_every_jni_reached_class_survived(self):
        reached = {
            "app.zxtune.core.Module",
            "app.zxtune.core.ResolvingException",
            "app.zxtune.core.jni.DataCallback",
            "app.zxtune.core.jni.DetectCallback",
            "app.zxtune.core.jni.JniApi",
            "app.zxtune.core.jni.JniModule",
            "app.zxtune.core.jni.JniOptions",
            "app.zxtune.core.jni.JniPlayer",
            "app.zxtune.core.jni.JniRuntimeException",
            "app.zxtune.core.jni.Plugins$Visitor",
            "app.zxtune.utils.ProgressCallback",
        }
        mapping = r8report.parse_mapping(os.path.join(self.RELEASE, "mapping.txt"))
        missing = sorted(reached - mapping.classes)
        self.assertEqual([], missing, "JNI-reached classes missing from the output")

    def test_inlined_and_gone_are_both_populated(self):
        mapping = r8report.parse_mapping(os.path.join(self.RELEASE, "mapping.txt"))
        self.assertGreater(len(mapping.inlined), 0)
        records = r8report.classify(mapping, os.path.join(self.RELEASE, "usage.txt"), "app.zxtune")
        info = r8report.summarize(records, "app.zxtune", "fatMinsdk16Release", "usage", "mapping", mapping)
        self.assertGreater(info["counts"]["INLINED"], 0)
        self.assertGreater(info["counts"]["GONE"], 0)
        self.assertEqual(info["members_total"], sum(
            len(r["members"]) for r in records))

    def test_manifest_referenced_classes_survive(self):
        # components the manifest names by string are instantiated reflectively,
        # so a keep-rule gap would only show up at runtime
        build_dir = os.path.join(self.ROOT, "pkg", "zxtune")
        manifest = r8report.find_manifest("fatMinsdk16Release", build_dir)
        if not manifest:
            self.skipTest("no merged manifest; run :zxtune:processFatMinsdk16ReleaseManifest first")
        referenced = r8report.parse_manifest(manifest)
        self.assertTrue(referenced)
        mapping = r8report.parse_mapping(os.path.join(self.RELEASE, "mapping.txt"))
        missing = sorted(referenced - mapping.classes)
        self.assertEqual([], missing, "manifest-referenced classes missing from the R8 output")


MANIFEST_FIXTURE = """\
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="app.zxtune">
    <permission android:name="app.zxtune.PERM" />
    <uses-permission android:name="app.zxtune.PERM" />
    <application android:name="app.zxtune.MainApplication"
        android:appComponentFactory="androidx.core.app.CoreComponentFactory">
        <activity android:name="app.zxtune.MainActivity" android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
            </intent-filter>
        </activity>
        <service android:name="app.zxtune.MainService" />
        <provider android:name="app.zxtune.playlist.Provider" android:authorities="app.zxtune.playlist" />
        <receiver android:name="app.zxtune.device.ui.WidgetHandler" />
        <provider android:name="androidx.startup.InitializationProvider">
            <meta-data android:name="app.zxtune.ui.utils.ThemeInitializer" android:value="androidx.startup" />
            <meta-data android:name="android.appwidget.provider" android:resource="@xml/widget_metadata" />
        </provider>
    </application>
</manifest>
"""


class ManifestRefsTest(unittest.TestCase):
    def test_class_names_are_collected_and_non_classes_ignored(self):
        tmp = tempfile.mkdtemp()
        path = _write(tmp, "AndroidManifest.xml", MANIFEST_FIXTURE)
        refs = r8report.parse_manifest(path)
        self.assertEqual(
            {
                "app.zxtune.MainApplication",
                "androidx.core.app.CoreComponentFactory",
                "app.zxtune.MainActivity",
                "app.zxtune.MainService",
                "app.zxtune.playlist.Provider",
                "app.zxtune.device.ui.WidgetHandler",
                "androidx.startup.InitializationProvider",
                "app.zxtune.ui.utils.ThemeInitializer",
            },
            refs,
        )
        # permissions, actions, authorities and non-startup meta-data are not classes
        for ignored in ("app.zxtune.PERM", "android.intent.action.MAIN",
                        "app.zxtune.playlist", "android.appwidget.provider"):
            self.assertNotIn(ignored, refs)

    def test_manifest_namespace_prefix_is_honored(self):
        tmp = tempfile.mkdtemp()
        path = _write(tmp, "AndroidManifest.xml", """\
<manifest xmlns:xyz="http://schemas.android.com/apk/res/android">
    <!-- <activity android:name="app.zxtune.Commented" /> -->
    <application xyz:name="app.zxtune.Alt" xyz:appComponentFactory="app.zxtune.Factory">
        <meta-data xyz:name="androidx.startup.Alt" xyz:value="androidx.startup" />
    </application>
</manifest>
""")
        self.assertEqual(
            {"app.zxtune.Alt", "app.zxtune.Factory", "androidx.startup.Alt"},
            r8report.parse_manifest(path),
        )

    def test_missing_manifest_classes_are_reported(self):
        tmp = tempfile.mkdtemp()
        mapping = r8report.parse_mapping(_write(tmp, "mapping.txt", MAPPING))
        usage = _write(tmp, "usage.txt", USAGE)
        records = r8report.classify(mapping, usage, "app.zxtune")
        manifest = _write(tmp, "AndroidManifest.xml", """\
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="app.zxtune">
    <application android:name="app.zxtune.core.jni.JniGC">
        <activity android:name="app.zxtune.core.jni.JniGC$HandleReference" />
        <activity android:name="app.zxtune.GoneActivity" />
    </application>
</manifest>
""")
        info = r8report.summarize(
            records, "app.zxtune", "test", "usage", "mapping", mapping, None, manifest
        )
        # both JniGC classes are in the output, GoneActivity is not
        self.assertEqual(["app.zxtune.GoneActivity"], info["missing_manifest"])
        summary = r8report.render_summary(info, records, set())
        self.assertIn("SUSPECT: 1 manifest-referenced classes missing from the output", summary)
        body = r8report.render(records, info, set())
        self.assertIn("1 classes the manifest names by string but the output left out:", body)
        self.assertIn("app.zxtune.GoneActivity", body)


ROOM_MAPPING = """\
app.zxtune.fs.ocremix.OcremixDatabaseDelegate_Impl -> a:
    1:1:void clearAllTables():1:1 -> b
"""

ROOM_USAGE = """\
app.zxtune.fs.ocremix.OcremixDatabaseDelegate_Impl:
    public void <init>()
    public void clearAllTables()
    protected androidx.room.InvalidationTracker createInvalidationTracker()
"""


class RoomCtorTest(unittest.TestCase):
    """Room resolves ``<DatabaseClass>_Impl`` reflectively, so its no-arg
    constructor is a runtime contract R8 cannot see."""

    def _records(self, mapping_text):
        tmp = tempfile.mkdtemp()
        mapping = r8report.parse_mapping(_write(tmp, "mapping.txt", mapping_text))
        usage = _write(tmp, "usage.txt", ROOM_USAGE)
        return r8report.classify(mapping, usage, "app.zxtune")

    def test_surviving_impl_without_constructor_is_reported(self):
        records = self._records(ROOM_MAPPING)
        self.assertEqual(
            ["app.zxtune.fs.ocremix.OcremixDatabaseDelegate_Impl"],
            r8report.room_stripped_constructors(records),
        )

    def test_impl_with_constructor_is_not_reported(self):
        healed = ROOM_MAPPING + "    1:1:void <init>():1:1 -> <init>\n"
        self.assertEqual([], r8report.room_stripped_constructors(self._records(healed)))

    def test_summary_signals_stripped_room_constructor(self):
        tmp = tempfile.mkdtemp()
        mapping = r8report.parse_mapping(_write(tmp, "mapping.txt", ROOM_MAPPING))
        usage = _write(tmp, "usage.txt", ROOM_USAGE)
        records = r8report.classify(mapping, usage, "app.zxtune")
        info = r8report.summarize(records, "app.zxtune", "test", "usage", "mapping", mapping)
        summary = r8report.render_summary(info, records, set())
        self.assertIn("SUSPECT: 1 Room database constructor(s) stripped from the output", summary)
        body = r8report.render(records, info, set())
        self.assertIn("app.zxtune.fs.ocremix.OcremixDatabaseDelegate_Impl.<init>()", body)

    def test_strict_fails_on_room_constructor(self):
        tmp = tempfile.mkdtemp()
        _write(tmp, "mapping.txt", ROOM_MAPPING)
        _write(tmp, "usage.txt", ROOM_USAGE)
        argv = ["--dir", tmp, "--prefix", "app.zxtune", "--build-dir", tmp, "--strict"]
        with open(os.devnull, "w", encoding="utf-8") as sink:
            with contextlib.redirect_stdout(sink), contextlib.redirect_stderr(sink):
                self.assertEqual(1, r8report.main(argv))


class SeedsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.mapping = r8report.parse_mapping(_write(self.tmp, "mapping.txt", MAPPING))
        self.seeds = _write(self.tmp, "seeds.txt", SEEDS)
        usage = _write(self.tmp, "usage.txt", USAGE)
        self.records = r8report.classify(self.mapping, usage, "app.zxtune")

    def test_seeded_classes_are_parsed(self):
        kept = r8report.parse_seeds(self.seeds, "app.zxtune")
        self.assertEqual(
            {
                "app.zxtune.core.jni.JniGC",
                "app.zxtune.core.jni.JniGC$HandleReference",
                "app.zxtune.core.jni.Plugins",
            },
            kept,
        )

    def test_member_level_seed_is_not_a_class_keep(self):
        # ``Class: <member>`` protects only that member; the class may still be
        # shrunk (e.g. AGP's default -keepclassmembers enum *), so it must not
        # read as a class-level seed that is missing from the output
        member_seeds = SEEDS + "app.zxtune.core.jni.JniPlayer: <init>()\n"
        path = _write(self.tmp, "member-seeds.txt", member_seeds)
        kept = r8report.parse_seeds(path, "app.zxtune")
        self.assertIn("app.zxtune.core.jni.JniGC", kept)
        self.assertNotIn("app.zxtune.core.jni.JniPlayer", kept)

    def test_seeded_but_absent_classes_are_missing(self):
        info = r8report.summarize(
            self.records, "app.zxtune", "test", "usage", "mapping", self.mapping, self.seeds
        )
        # Plugins is a keep rule seed but entirely absent from the output
        self.assertEqual(["app.zxtune.core.jni.Plugins"], info["missing_seeds"])

    def test_summary_signals_missing_seeds(self):
        info = r8report.summarize(
            self.records, "app.zxtune", "test", "usage", "mapping", self.mapping, self.seeds
        )
        summary = r8report.render_summary(info, self.records, set())
        self.assertIn("SUSPECT: 1 kept app classes missing from the output", summary)


class StrictTest(unittest.TestCase):
    def _run(self, tmp, seeds, strict):
        _write(tmp, "mapping.txt", MAPPING)
        _write(tmp, "usage.txt", USAGE)
        _write(tmp, "seeds.txt", seeds)
        argv = ["--dir", tmp, "--prefix", "app.zxtune", "--build-dir", tmp]
        if strict:
            argv.append("--strict")
        with open(os.devnull, "w", encoding="utf-8") as sink:
            with contextlib.redirect_stdout(sink), contextlib.redirect_stderr(sink):
                return r8report.main(argv)

    def test_strict_fails_when_a_kept_class_is_stripped(self):
        # Plugins is seeded but absent from the output
        self.assertEqual(1, self._run(tempfile.mkdtemp(), SEEDS, strict=True))

    def test_without_strict_a_stripped_class_is_only_reported(self):
        self.assertEqual(0, self._run(tempfile.mkdtemp(), SEEDS, strict=False))

    def test_strict_passes_when_nothing_is_missing(self):
        kept = "".join(
            line for line in SEEDS.splitlines(True) if not line.startswith("app.zxtune.core.jni.Plugins")
        )
        self.assertEqual(0, self._run(tempfile.mkdtemp(), kept, strict=True))


if __name__ == "__main__":
    unittest.main(verbosity=2)
