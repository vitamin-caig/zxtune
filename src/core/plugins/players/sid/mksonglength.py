#!/usr/bin/python

import binascii
import math
import re
import sys

class SongLengths:
  LINEFORMAT = re.compile(r'([\da-f]{32})=(.*)')
  TIMEFORMAT = re.compile(r'(\d+):(\d+)')
  
  def __init__(self, stream):
    self._songs = []
    self._durations = []
    md5s = set()
    crcs = set()
    for line in stream:
      props = SongLengths.LINEFORMAT.match(line)
      if props:
        md5 = props.group(1)
        if md5 in md5s:
          raise Exception('Duplicated song with md5=' + md5)
        crc = binascii.crc32(md5.encode('utf-8')) & 0xffffffff
        if crc in crcs:
          raise Exception('Collision for md5=' + md5 + ' crc=' + crc)
        times = [SongLengths._timeFromString(strTime) for strTime in props.group(2).split() if len(strTime) != 0]
        self._songs.append({'crc': crc, 'md5': md5, 'duration': times})
        self._durations.extend(times)
    self._songs.sort(key = lambda x: x['crc'])
    self._durations.sort()

  def dumpDatabase(self, stream):
    for song in self._songs:
      crc = song['crc']
      md5 = song['md5']
      print('//' + md5, file = stream)
      for duration in song['duration']:
        print('{0x%08x, %d},' % (crc, duration), file = stream)

  def dumpStatistic(self, stream):
    totalCount = len(self._durations)
    totalDuration = sum(self._durations)
    print('Average=%d (%d/%d)' % (totalDuration / totalCount, totalDuration, totalCount), file = stream)
    for percent in (50, 60, 75, 80, 90, 95):
      pos = float(percent * totalCount) / 100
      percentile = self._durations[math.ceil(pos)]
      print('%dth percentile=%d' % (percent, percentile), file = stream)

  @staticmethod
  def _timeFromString(timeStr):
    fmt = SongLengths.TIMEFORMAT.match(timeStr)
    if not fmt:
      raise Exception('Invalid time value: ', timeStr)
    res = 60 * int(fmt.group(1)) + int(fmt.group(2))
    return res if res != 0 else 1

def main():
  songs = SongLengths(sys.stdin)
  songs.dumpDatabase(sys.stdout)
  songs.dumpStatistic(sys.stderr)

if __name__ == '__main__':
  main()
