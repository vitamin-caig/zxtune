#!/usr/bin/python

import binascii
import re
import sys

class Song:
  def __init__(self, md5, durations):
    self.key = binascii.crc32(md5.encode())
    self.md5 = md5
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
    for durations in reversed(sorted(allDurations)):
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
      assert d < 1 << 14
      if d < 128:
        yield d
      else:
        yield 128 | (d & 127)
        yield d >> 7

class EntriesTable:
  GROUP_SIZE_MASK = 0xffff0000
  GROUP_SIZE_SHIFT = 16

  @staticmethod
  def dump(songs):
    print(f'constexpr uint32_t GROUP_SIZE_SHIFT = {EntriesTable.GROUP_SIZE_SHIFT};')
    print(f'constexpr uint32_t GROUP_SIZE_MASK = {EntriesTable.GROUP_SIZE_MASK:#x};')
    print(f'constexpr uint32_t VALUE_MASK = 0xffff;')
    print('constexpr uint64_t ENTRIES[] = {')
    for crc in sorted(songs.keys()):
      assert crc > 0
      assert crc < 0x100000000
      song = songs[crc]
      key = (crc << 32) | EntriesTable._encode(song)
      print(f' 0x{key:016x}, // {song.md5}={EntriesTable._describe(song)}')
    print(f'}}; // {len(songs)} entries = {8 * len(songs)} bytes')

  @staticmethod
  def _encode(song):
    elements = len(song.durations)
    if elements == 1:
      res = song.durations[0]
      assert res < EntriesTable.GROUP_SIZE_MASK
      return res
    else:
      # if more than 16 bit required, reduce bits for group size
      assert song.groupOffset < EntriesTable.GROUP_SIZE_MASK
      return song.groupOffset | (elements << EntriesTable.GROUP_SIZE_SHIFT)

  @staticmethod
  def _describe(song):
    elements = len(song.durations)
    return f'{song.durations[0]}s' if elements == 1 else f'{elements}@{song.groupOffset:#04x}'


class SongLengths:
  LINEFORMAT = re.compile(r'([\da-f]{32})=(.*)')
  TIMEFORMAT = re.compile(r'(\d+):(\d+)')
  
  def __init__(self):
    self._md5s = dict()
    self._crcs = dict()
    self._collisions = 0

  def read(self, stream):
    for line in stream:
      props = SongLengths.LINEFORMAT.match(line)
      if props:
        md5 = props.group(1)
        durations = tuple(SongLengths._timeFromString(strTime) for strTime in props.group(2).split() if len(strTime) != 0)
        oldSong = self._md5s.get(md5)
        if not oldSong:
          song = Song(md5, durations)
          self._md5s[md5] = song
          crc = song.key
          prev = self._crcs.setdefault(crc, song)
          if prev != song:
            print(f'Collison for {crc:#x} {prev.md5} != {song.md5}', file = sys.stderr)
            self._collisions += 1
          continue
        if len(oldSong.durations) != len(durations):
          raise Exception(f'Conflicting entries count md5={md5}')
        elif oldSong.durations != durations:
          print(f'Update {md5} {oldSong.durations} -> {durations}', file = sys.stderr)
          oldSong.durations = durations

  def dumpStatistic(self):
    songs = self._md5s.values()
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
    LengthsTable.dump(self._md5s)

  def dumpEntries(self):
    EntriesTable.dump(self._crcs)

  @staticmethod
  def _timeFromString(timeStr):
    fmt = SongLengths.TIMEFORMAT.match(timeStr)
    if not fmt:
      raise Exception('Invalid time value: ', timeStr)
    res = 60 * int(fmt.group(1)) + int(fmt.group(2))
    assert res != 0
    return res

def main():
  songs = SongLengths()
  for path in sys.argv[1:]:
    songs.read(open(path))
  songs.dumpStatistic()
  songs.dumpGroupDurations()
  songs.dumpEntries()

if __name__ == '__main__':
  main()
