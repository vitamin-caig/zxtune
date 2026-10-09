#!/usr/bin/env python3
"""Classify the symbols R8 removed for one variant, per variant directory.

Reads the R8 diagnostics AGP writes to ``pkg/<module>/outputs/mapping/<variant>/``
(``mapping.txt`` and ``usage.txt``) and decides, for every symbol ``usage.txt``
reports, which of these three states it ended up in:

  RETAINED  the symbol still exists in the output (renamed and/or optimized)
  INLINED   the symbol has no body of its own left, but its code was folded
            into a caller, which mapping.txt records as an extra frame
  GONE      nothing refers to the symbol any more, and it is absent from the
            output

``usage.txt`` carries no reason tags, so this cannot be read off it directly;
the discriminator is the frame groups in ``mapping.txt``. Consecutive method
lines that share an output range and a target describe one inlined call chain,
and only the last frame of such a group is a real method of the output class.

Even a whole class that left the output is checked against those groups: its
members may still be folded into the callers that used them, and the report
lists them per removed class. Fields are the exception - a compiled-out
``static final`` constant leaves no mapping frame at all, so a folded field is
indistinguishable from a removed one here.

Besides the usage/mapping pair, the merged ``AndroidManifest.xml`` is consulted
when it can be found: components, ``appComponentFactory`` and ``androidx.startup``
initializers are looked up by string at runtime, so any of them missing from the
output is a latent crash the static analysis above cannot see. Room's generated
``<DatabaseClass>_Impl`` classes are covered the same way: Room resolves them by
string, so a surviving ``*_Impl`` that lost its no-arg constructor is reported.

With ``--strict`` the process exits non-zero when a kept class, a
manifest-referenced class or a Room database constructor is missing, so a build
can stop on it (the gradle task relies on this).
"""

import argparse
import glob
import os
import re
import sys

# The output range and target shared by every frame of one inlined call chain.
RANGE_SEP = "\x00"
GROUP_SEP = "\x00"


class Mapping:
    """Everything of interest that survived, indexed by ``<owner>.<member>``."""

    def __init__(self):
        self.classes = set()
        self.retained = set()
        self.inlined = set()
        self.fields = set()
        self.inlined_owners = {}

    def verdicts(self, sig):
        """``both`` when a symbol is retained at one call site and inlined at another."""
        has_retained = sig in self.retained
        has_inlined = sig in self.inlined
        if has_retained and has_inlined:
            return "both"
        if has_retained:
            return "RETAINED"
        if has_inlined:
            return "INLINED"
        return None


def _class_header(line):
    """Return the original class name if the line opens a class block."""
    if not line or line[0] in " \t#":
        return None
    if not line.endswith(":"):
        return None
    name, sep, _ = line[:-1].partition(" ->")
    if not sep or not name or any(c in name for c in " \t"):
        return None
    return name


def _frame(line):
    """Parse a method mapping line into ``(out_range, target, signature)``.

    ``signature`` is ``<owner>.<name>`` with the owner taken from the frame
    itself when present. Frames that name only a method carry no owner, because
    they belong to the class whose block encloses them; the caller supplies it.
    """
    body = line.strip()
    head, sep, tail = body.partition(" ->")
    if not sep:
        return None
    target = tail.strip()
    # leading <outStart>:<outEnd>: is the only part without a space
    parts = head.split(":", 2)
    if len(parts) < 3:
        return None
    out_range = parts[0] + ":" + parts[1]
    rest = parts[2].strip()
    if "(" not in rest:
        return None
    # <retType> [<owner>.]<name>(<params>)[:<origStart>[:<origEnd>]]
    decl, paren, _ = rest.partition("(")
    decl = decl.rsplit(":", 1)[0].strip()
    name = decl.rpartition(" ")[2].strip() if " " in decl else decl
    if not name:
        return None
    return out_range, target, name


def _is_field(line):
    """True for ``    <type> <name> -> <new>``, i.e. a field rather than a frame."""
    if not line.startswith(" "):
        return False
    if line.lstrip().startswith("#"):
        return False
    body = line.strip()
    head, sep, _ = body.partition(" ->")
    if not sep:
        return False
    head = head.strip()
    if not head or head[0].isdigit():
        return False
    return " " in head


def parse_mapping(path):
    mapping = Mapping()
    owner = None
    group_key = None
    # signatures of the frames seen so far for the current group
    buffered = []

    def flush():
        # every frame but the last of a group was inlined into the last one
        if not buffered:
            return
        for sig in buffered[:-1]:
            mapping.inlined.add(sig)
            owner, _, name = sig.rpartition(".")
            mapping.inlined_owners.setdefault(owner, set()).add(name)
        mapping.retained.add(buffered[-1])
        del buffered[:]

    with open(path, encoding="utf-8", errors="replace") as src:
        for line in src:
            line = line.rstrip("\n")
            header = _class_header(line)
            if header is not None:
                flush()
                group_key = None
                owner = header
                mapping.classes.add(header)
                continue
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                # comments (sourceFile, residual signature) interleave with
                # frames of the same group and must not break it
                continue
            if _is_field(line):
                body = line.strip().split(" ->")[0].strip().split()
                mapping.fields.add(owner + "." + body[-1])
                continue
            parsed = _frame(line)
            if parsed is None:
                flush()
                group_key = None
                continue
            out_range, target, name = parsed
            # a frame names its owner unless the method belongs to the class whose
            # block encloses it, in which case the owner has to be supplied here
            if "." in name:
                sig = name
            else:
                sig = (owner or "?") + "." + name
            key = out_range + RANGE_SEP + target
            if key != group_key:
                flush()
                group_key = key
            buffered.append(sig)
    flush()
    return mapping


class Member(object):
    __slots__ = ("owner", "name", "text", "is_field", "verdict")

    def __init__(self, owner, name, text, is_field):
        self.owner = owner
        self.name = name
        self.text = text
        self.is_field = is_field
        self.verdict = None

    @property
    def sig(self):
        return self.owner + "." + self.name


def parse_usage(path, prefix):
    """Return one record per removed class, plus one per class whose members were removed.

    A bare class line means the whole class left the output and no members follow
    for it; a ``Class:`` header means the class survived and the indented lines
    that follow are its members.
    """
    records = []
    current = None
    with open(path, encoding="utf-8", errors="replace") as src:
        for line in src:
            line = line.rstrip("\n")
            if not line.strip():
                continue
            if line[0].isspace():
                if current is None:
                    continue
                text = line.strip()
                head = text.split("(", 1)[0]
                parts = head.split()
                if not parts:
                    continue
                current["members"].append(
                    Member(current["name"], parts[-1], text, "(" not in text)
                )
                continue
            current = None
            if line.endswith(":"):
                name = line[:-1]
                if name.startswith(prefix):
                    current = {"name": name, "removed": False, "members": []}
                    records.append(current)
            elif line.startswith(prefix):
                records.append({"name": line, "removed": True, "members": []})
    return records


def parse_seeds(path, prefix):
    """Return the classes the keep rules protected in whole.

    ``seeds.txt`` lists every symbol the keep rules passed to R8. Only a bare
    entry (``Class``) keeps the whole class; ``Class: <member>`` keeps just that
    member, so the class itself may still be shrunk or dropped. The AGP default
    ``-keepclassmembers enum *`` yields member lines for every enum, and treating
    those as class keeps reports harmless enum unboxing as a missing class.
    """
    kept = set()
    with open(path, encoding="utf-8", errors="replace") as src:
        for line in src:
            line = line.rstrip("\n")
            if not line.strip() or line[0].isspace():
                continue
            if not line.startswith(prefix):
                continue
            if ":" in line:
                continue
            kept.add(line)
    return kept


# Android manifest elements whose ``android:`` attributes name a class the
# platform instantiates reflectively; nothing references them statically.
# ``meta-data`` with value ``androidx.startup`` names an initializer class.
_MANIFEST_CLASS_ATTRS = {
    "application": ("name", "appComponentFactory"),
    "activity": ("name",),
    "activity-alias": ("name", "targetActivity"),
    "service": ("name",),
    "provider": ("name",),
    "receiver": ("name",),
}
_ANDROID_NS = "http://schemas.android.com/apk/res/android"

# Match any ``xmlns:PREFIX`` bound to the android namespace, so a manifest that
# names it something other than ``android`` is still read correctly.
_ANDROID_XMLNS_RE = re.compile(
    r'xmlns:([A-Za-z_][\w.-]*)\s*=\s*"%s"' % re.escape(_ANDROID_NS)
)
_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)
# A start or empty tag: the leading ``(?![/!?])`` skips closing tags, comments
# and the ``<?xml?>`` declaration; ``[^>]*`` is safe because AGP never emits a
# raw ``>`` inside a manifest attribute value.
_ELEMENT_RE = re.compile(r"<(?![/!?])([A-Za-z_][\w.-]*)([^>]*)>")
_ATTR_RE = re.compile(r'([A-Za-z_][\w.:-]*)\s*=\s*"([^"]*)"')


def parse_manifest(path):
    """Return the classes an Android manifest names by string.

    The manifest is machine-generated XML, so a local parser avoids depending on
    the ``xml`` package, which minimal CI/stripped interpreters ship without.
    """
    with open(path, "r", encoding="utf-8") as manifest:
        text = _COMMENT_RE.sub("", manifest.read())
    match = _ANDROID_XMLNS_RE.search(text)
    ns = (match.group(1) if match else "android") + ":"
    refs = set()
    for element in _ELEMENT_RE.finditer(text):
        tag = element.group(1)
        attrs = dict(_ATTR_RE.findall(element.group(2)))
        class_attrs = _MANIFEST_CLASS_ATTRS.get(tag)
        if class_attrs:
            for attr in class_attrs:
                value = attrs.get(ns + attr)
                if value:
                    refs.add(value)
        elif tag == "meta-data" and attrs.get(ns + "value") == "androidx.startup":
            name = attrs.get(ns + "name")
            if name:
                refs.add(name)
    return refs


def find_manifest(variant, build_dir):
    """Locate the merged manifest AGP writes for a variant, if it exists."""
    if not variant:
        return None
    matches = sorted(glob.glob(
        os.path.join(build_dir, "intermediates", "merged_manifests", variant, "*", "AndroidManifest.xml")
    ))
    return matches[0] if matches else None


def classify(mapping, usage_path, prefix):
    """Return per-class records with every member verdict filled in."""
    records = parse_usage(usage_path, prefix)
    for record in records:
        record["class_gone"] = record["removed"] and record["name"] not in mapping.classes
        record["inlined_members"] = tuple(sorted(mapping.inlined_owners.get(record["name"], ())))
        for member in record["members"]:
            sig = member.sig
            if member.is_field:
                member.verdict = "RETAINED" if sig in mapping.fields else "GONE"
            else:
                member.verdict = mapping.verdicts(sig) or "GONE"
    return records


def _overload_collisions(records, mapping):
    """Names where one overload survived and another was removed.

    mapping.txt records frames by name, not by full signature, so a removed
    overload sharing a name with a retained one cannot be told apart.
    """
    collisions = set()
    for record in records:
        for member in record["members"]:
            if member.is_field:
                continue
            if member.verdict == "RETAINED":
                collisions.add(member.sig)
    return collisions


_ROOM_IMPL_SUFFIX = "_Impl"


def room_stripped_constructors(records):
    """Room database implementations that lost their no-arg constructor.

    Room never references ``<DatabaseClass>_Impl`` statically: it builds the name
    from strings and instantiates it through ``Class.forName(...)
    .getDeclaredConstructor().newInstance()``. R8 cannot see that, and Room's own
    consumer rule (``-keep class * extends androidx.room.RoomDatabase``) keeps the
    class name but not the constructor, so the app level
    ``-keep class * extends androidx.room.RoomDatabase { <init>(); }`` rule is what
    keeps it alive. Once ``<init>()`` is gone the whole ``super()`` chain collapses
    and every DB-backed query crashes, yet usage.txt only lists a member removal, so
    no class-level signal fires. Report each surviving ``*_Impl`` whose no-arg
    constructor was removed.
    """
    flagged = []
    for record in records:
        if record["removed"] or not record["name"].endswith(_ROOM_IMPL_SUFFIX):
            continue
        for member in record["members"]:
            if member.name == "<init>" and member.verdict == "GONE" \
                    and member.text.rstrip().endswith("<init>()"):
                flagged.append(record["name"])
                break
    return sorted(flagged)


def summarize(records, prefix, variant, usage_path, mapping_path, mapping, seeds_path=None, manifest_path=None):
    counts = {"RETAINED": 0, "INLINED": 0, "GONE": 0, "both": 0}
    classes_gone = 0
    classes_total = 0
    members_total = 0
    classes_weakened = 0
    for record in records:
        if record["removed"]:
            classes_total += 1
            if record["class_gone"]:
                classes_gone += 1
            # members of a removed class leave no usage.txt members, but their
            # inlined frames in mapping.txt still prove the code survived
            counts["INLINED"] += len(record["inlined_members"])
        else:
            # a retained class that still lost members; usage.txt leaves the
            # survivors out, so only the removals are about to be counted
            if any(member.verdict == "GONE" for member in record["members"]):
                classes_weakened += 1
        for member in record["members"]:
            members_total += 1
            counts[member.verdict] = counts.get(member.verdict, 0) + 1
    missing_seeds = set()
    if seeds_path and os.path.isfile(seeds_path):
        missing_seeds = sorted(parse_seeds(seeds_path, prefix) - mapping.classes)
    missing_manifest = []
    if manifest_path and os.path.isfile(manifest_path):
        missing_manifest = sorted(parse_manifest(manifest_path) - mapping.classes)
    return {
        "prefix": prefix,
        "variant": variant,
        "usage": usage_path,
        "mapping": mapping_path,
        "classes_total": classes_total,
        "classes_gone": classes_gone,
        "classes_weakened": classes_weakened,
        "members_total": members_total,
        "counts": counts,
        "missing_seeds": missing_seeds,
        "missing_manifest": missing_manifest,
        "missing_room_ctors": room_stripped_constructors(records),
    }


def render(records, info, collisions):
    out = []
    out.append("R8 symbol report for %s" % info["variant"])
    out.append("  usage:   %s" % info["usage"])
    out.append("  mapping: %s" % info["mapping"])
    out.append("  package: %s*" % info["prefix"])
    out.append("")
    out.append(
        "A symbol is INLINED when its code was folded into a caller, which "
        "mapping.txt"
    )
    out.append(
        "records as an extra frame of the same output range; RETAINED when it is "
        "still a member"
    )
    out.append("of the output; GONE when nothing refers to it any more.")
    out.append("")
    out.append(
        "  classes in removal set : %d (fully gone %d)" % (info["classes_total"], info["classes_gone"])
    )
    out.append("  classes with removals  : %d" % info["classes_weakened"])
    out.append("  members                : %d" % info["members_total"])
    for verdict in ("RETAINED", "INLINED", "both", "GONE"):
        if info["counts"].get(verdict):
            out.append("    %-21s %d" % (verdict, info["counts"][verdict]))
    if info["missing_seeds"]:
        out.append("")
        out.append("  %d app classes the keep rules protected but the output left out:" % len(info["missing_seeds"]))
        for sig in info["missing_seeds"]:
            out.append("    %s" % sig)
    if info["missing_manifest"]:
        out.append("")
        out.append("  %d classes the manifest names by string but the output left out:" % len(info["missing_manifest"]))
        out.append("  (instantiated reflectively, so only a runtime failure would show):")
        for sig in info["missing_manifest"]:
            out.append("    %s" % sig)
    if info["missing_room_ctors"]:
        out.append("")
        out.append("  %d Room database constructors stripped from the output:" % len(info["missing_room_ctors"]))
        out.append("  (Room instantiates *_Impl by name via reflection, so only a runtime crash would show):")
        for sig in info["missing_room_ctors"]:
            out.append("    %s.<init>()" % sig)
    if collisions:
        out.append("")
        out.append(
            "  %d removed members share a name with a retained one, so their "
            "individual" % len(collisions)
        )
        out.append("  overload is indistinguishable from mapping.txt:")
        for sig in sorted(collisions)[:20]:
            out.append("    %s" % sig)
        if len(collisions) > 20:
            out.append("    ... and %d more" % (len(collisions) - 20))
    out.append("")
    out.append("=" * 100)
    for record in records:
        if record["removed"]:
            inlined = record["inlined_members"]
            suffix = " (inlined: %s)" % ", ".join(inlined) if inlined else " (removed in whole)"
            out.append("%s  %s%s" % ("GONE".ljust(9), record["name"], suffix))
            continue
        for member in record["members"]:
            out.append(
                "%s  %s | %s" % (member.verdict.ljust(9), member.sig, member.text)
            )
    return "\n".join(out) + "\n"


# Standard-library packages whose removal would not tell us anything about the
# app: R8 never removes these statically, and usage.txt would not list them
# under a shorter prefix, but they appear under the loose default below.
STANDARD_PREFIXES = ("java.", "javax.", "android.", "kotlin.", "kotlinx.", "kotlinx.cinterop.", "org.")


def _removed_classes(records):
    return sorted(
        record["name"]
        for record in records
        if record["removed"] and not record["name"].startswith(STANDARD_PREFIXES)
    )


def _weakened_classes(records):
    """Returns (name, removed member names) for retained classes that lost members."""
    return sorted(
        (
            record["name"],
            tuple(sorted(member.name for member in record["members"] if member.verdict == "GONE")),
        )
        for record in records
        if not record["removed"]
        and not record["name"].startswith(STANDARD_PREFIXES)
        and any(member.verdict == "GONE" for member in record["members"])
    )


def _forest(records):
    """Return a nested dict of package components; class leaves hold
    ``(removed_members, inlined_members)``."""
    tree = {}
    for name, removed_members in _weakened_classes(records):
        node = tree
        parts = name.split(".")
        for part in parts[:-1]:
            child = node.get(part)
            if not isinstance(child, dict):
                child = {}
                node[part] = child
            node = child
        node[parts[-1]] = (removed_members, ())
    for record in records:
        if not record["removed"] or record["name"].startswith(STANDARD_PREFIXES):
            continue
        node = tree
        parts = record["name"].split(".")
        for part in parts[:-1]:
            child = node.get(part)
            if not isinstance(child, dict):
                child = {}
                node[part] = child
            node = child
        node[parts[-1]] = ((), record.get("inlined_members", ()))
    return tree


def _direct_classes(node):
    return sum(1 for child in node.values() if not isinstance(child, dict))


def _render_tree(node, indent, lines):
    entries = sorted(node.items())
    total = len(entries)
    for index, (name, child) in enumerate(entries):
        last = index + 1 == total
        branch = "`- " if last else "|- "
        if isinstance(child, dict):
            lines.append("%s%s%s/ (%d)" % (indent, branch, name, _direct_classes(child)))
            _render_tree(child, indent + ("   " if last else "|  "), lines)
        else:
            removed, inlined = child
            if removed:
                suffix = " (removed: %s)" % ", ".join(removed[:8])
                if len(removed) > 8:
                    suffix += " ..."
            elif inlined:
                suffix = " (inlined: %s)" % ", ".join(inlined)
            else:
                suffix = " (removed in whole)"
            lines.append("%s%s%s%s" % (indent, branch, name, suffix))


def _removed_forest_lines(records):
    lines = []
    _render_tree(_forest(records), "", lines)
    return lines


def render_summary(info, records, collisions):
    lines = []
    counts = info["counts"]
    lines.append(
        "r8report: %s -> %d classes (%d fully gone), %d members"
        % (info["variant"], info["classes_total"], info["classes_gone"], info["members_total"])
    )
    lines.append(
        "  RETAINED %d | INLINED %d | both %d | GONE %d"
        % (counts.get("RETAINED", 0), counts.get("INLINED", 0), counts.get("both", 0), counts.get("GONE", 0))
    )
    removed = _removed_classes(records)
    weakened = [name for name, _ in _weakened_classes(records)]
    if removed or weakened:
        lines.append("  removed classes (%d) + weakened classes (%d):" % (len(removed), len(weakened)))
        lines.extend(_removed_forest_lines(records))
    if info["missing_seeds"]:
        lines.append("  SUSPECT: %d kept app classes missing from the output (see report)" % len(info["missing_seeds"]))
    if info["missing_manifest"]:
        lines.append(
            "  SUSPECT: %d manifest-referenced classes missing from the output (see report)"
            % len(info["missing_manifest"])
        )
    if info["missing_room_ctors"]:
        lines.append(
            "  SUSPECT: %d Room database constructor(s) stripped from the output (see report)"
            % len(info["missing_room_ctors"])
        )
    if collisions:
        lines.append("  %d overload name collisions (see report)" % len(collisions))
    return "\n".join(lines)


def find_variant_dir(variant, build_dir):
    if variant is None:
        return None
    return os.path.join(build_dir, "outputs", "mapping", variant)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", help="variant name, e.g. <Flavor><BuildType>")
    parser.add_argument("--dir", help="explicit directory holding usage.txt and mapping.txt")
    parser.add_argument(
        "--build-dir",
        default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "pkg", "zxtune"),
        help="module build dir (default: repo pkg/zxtune)",
    )
    parser.add_argument("--prefix", required=True, help="package prefix to report on")
    parser.add_argument("--manifest", help="merged AndroidManifest.xml to check (default: auto-discover)")
    parser.add_argument("--out", help="report file (default: <dir>/report.txt)")
    parser.add_argument("--quiet", action="store_true", help="write the report but print nothing")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="exit non-zero when a SUSPECT signal fires, so a build can stop on a stripped class",
    )
    args = parser.parse_args(argv)

    directory = args.dir or find_variant_dir(args.variant, os.path.abspath(args.build_dir))
    if not directory:
        parser.error("need --variant or --dir")
    variant = args.variant or os.path.basename(directory)
    build_dir = os.path.abspath(args.build_dir)
    usage_path = os.path.join(directory, "usage.txt")
    mapping_path = os.path.join(directory, "mapping.txt")
    seeds_path = os.path.join(directory, "seeds.txt")
    manifest_path = args.manifest or find_manifest(variant, build_dir)
    for path in (usage_path, mapping_path):
        if not os.path.isfile(path):
            parser.error("missing %s - run the R8 task for this variant first" % path)
    out_path = args.out or os.path.join(directory, "report.txt")

    mapping = parse_mapping(mapping_path)
    records = classify(mapping, usage_path, args.prefix)
    info = summarize(
        records, args.prefix, variant, usage_path, mapping_path, mapping, seeds_path, manifest_path
    )
    collisions = _overload_collisions(records, mapping)

    with open(out_path, "w", encoding="utf-8") as dst:
        dst.write(render(records, info, collisions))
    if not args.quiet:
        print(render_summary(info, records, collisions))
        print("  report: %s" % out_path)
    suspect = len(info["missing_seeds"]) + len(info["missing_manifest"]) + len(info["missing_room_ctors"])
    if suspect and args.strict:
        print(
            "r8report: %d important class(es) stripped from the output; fix the keep rules (%s)"
            % (suspect, out_path),
            file=sys.stderr,
        )
    return 1 if (args.strict and suspect) else 0


if __name__ == "__main__":
    sys.exit(main())
