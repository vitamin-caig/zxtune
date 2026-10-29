#!/usr/bin/python

import re
import sys

class Song:
  def __init__(self, key, durations):
    self.key = key
    self.durations = durations

class LengthsTable:
  @staticmethod
  def dump(songs):
    groups = LengthsTable._buildGroups(songs)
    for song in songs.values():
      if len(song.durations) > 1:
        song.groupOffset = groups[song.durations]

  @staticmethod
  def _buildGroups(songs):
    allDurations = frozenset(song.durations for song in songs.values() if len(song.durations) != 1)
    offset = 0
    result = dict()
    print('constexpr uint8_t GROUPS[] = {', end='')
    prev = None
    prevOffset = None
    for durations in sorted(allDurations, reverse=True):
      if prev and len(prev) > len(durations) and prev[0:len(durations)] == durations:
        print(f'\n// reuse +{prevOffset:#x} for {durations}', end='')
        result[durations] = prevOffset
        continue
      result[durations] = offset
      prev = durations
      prevOffset = offset
      print(f'\n// +{offset:#x} {durations}')
      for b in LengthsTable._compress(durations):
        print(hex(b), end=',')
        offset += 1
    print(f'\n}}; // {offset} bytes total')
    return result

  @staticmethod
  def _compress(durations):
    total = len(durations)
    assert total > 1
    for d in durations:
      assert d >= 0
      while d >= 128:
        yield 128 | (d & 127)
        d >>= 7
      yield d

class EntriesTable:
  VALUE_BITS = 20
  VALUE_MASK = (1 << VALUE_BITS) - 1
  GROUP_SIZE_SHIFT = VALUE_BITS
  GROUP_SIZE_BITS = 32 - VALUE_BITS
  GROUP_SIZE_MASK = ((1 << GROUP_SIZE_BITS) - 1) << GROUP_SIZE_SHIFT

  @staticmethod
  def dump(songs):
    print(f'constexpr uint32_t VALUE_MASK = {EntriesTable.VALUE_MASK:#x};')
    print('constexpr uint64_t ENTRIES[] = {')
    for crc in sorted(songs.keys()):
      assert crc > 0
      assert crc < 0x100000000
      song = songs[crc]
      key = (crc << 32) | EntriesTable._encode(song)
      print(f' 0x{key:016x}, // {crc:08x}={EntriesTable._describe(song)}')
    print(f'}}; // {len(songs)} entries = {8 * len(songs)} bytes')

  @staticmethod
  def _encode(song):
    elements = len(song.durations)
    if elements == 1:
      res = song.durations[0]
      assert res <= EntriesTable.VALUE_MASK
      return res
    else:
      assert song.groupOffset <= EntriesTable.VALUE_MASK
      assert elements < (1 << EntriesTable.GROUP_SIZE_BITS)
      return song.groupOffset | (elements << EntriesTable.GROUP_SIZE_SHIFT)

  @staticmethod
  def _describe(song):
    elements = len(song.durations)
    return f'{song.durations[0]}' if elements == 1 else f'{elements}@{song.groupOffset:#04x}'

class SongLengths:
  LINEFORMAT = re.compile(r'TIMEDB_ENTRY\(\s*([\da-fA-F]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,')
  MAX_TRACK = (1 << EntriesTable.GROUP_SIZE_BITS) - 1

  def __init__(self):
    self._md5s = dict()
    self._crcs = dict()
    self._collisions = 0

  def read(self, stream):
    for line in stream:
      props = SongLengths.LINEFORMAT.search(line)
      if props:
        key = int(props.group(1), 16)
        track = int(props.group(2))
        duration = int(props.group(3))
        if track < 1 or track > SongLengths.MAX_TRACK:
          raise Exception(f'Invalid track {track} for {key:#x}')
        if duration < 1 or duration > EntriesTable.VALUE_MASK:
          raise Exception(f'Invalid duration {duration} for {key:#x}/{track}')
        tracks = self._md5s.get(key)
        if not tracks:
          tracks = self._md5s[key] = dict()
        prev = tracks.setdefault(track, duration)
        if prev != duration:
          print(f'Update {key:#x}/{track} {prev} -> {duration}', file = sys.stderr)
          tracks[track] = duration

  def _songs(self):
    if not self._crcs:
      for key, tracks in self._md5s.items():
        # Zero marks a track number absent from the database; lookup maps idx directly
        durations = tuple(tracks.get(track, 0) for track in range(1, max(tracks) + 1))
        self._crcs[key] = Song(key, durations)
    return self._crcs

  def dumpStatistic(self):
    songs = self._songs().values()
    print('// Statistics')
    print(f'// Entries={len(songs)}')
    print(f'// Collisions={self._collisions}')
    songSizes = sorted(frozenset(len(s.durations) for s in songs))
    print(f'// TracksCount={songSizes[:10]}..{songSizes[-10:]}')
    durations = sorted(sum([s.durations for s in songs], ()))
    uniqDurations = sorted(frozenset(durations))
    print('// Durations:')
    print(f'//  range={uniqDurations[:10]}..{uniqDurations[-10:]}')
    print(f'//  uniqueValues={len(uniqDurations)}')
    print(f'//  uniqueGroups={len(frozenset(s.durations for s in songs))}')
    singles = frozenset(s.durations[0] for s in songs if len(s.durations) == 1)
    print(f'//  uniqueSingleValues={len(singles)}, max={max(singles)}')
    multis = frozenset(s.durations for s in songs if len(s.durations) > 1)
    print(f'//  uniqueMultiGroups={len(multis)}')
    print(f'//  uniqueMultiGroupsTotalElements={sum([len(x) for x in multis])}')
    totalCount = len(durations)
    totalDuration = sum(durations)
    print(f'//  average={totalDuration / totalCount} ({totalDuration}/{totalCount})')
    for percent in [50, 60, 75, 80, 90, 95, 99]:
      pos = percent * totalCount // 100
      percentile = durations[pos]
      print(f'//  p{percent}={percentile}')

  def dumpGroupDurations(self):
    LengthsTable.dump(self._songs())

  def dumpEntries(self):
    EntriesTable.dump(self._songs())

def main():
  songs = SongLengths()
  for path in sys.argv[1:]:
    songs.read(open(path))
  songs.dumpStatistic()
  songs.dumpGroupDurations()
  songs.dumpEntries()

if __name__ == '__main__':
  main()
