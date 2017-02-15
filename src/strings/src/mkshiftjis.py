#!/usr/bin/python

import binascii
import math
import re
import sys

class ShiftJIS:
  LINEFORMAT = re.compile(r'.*0x([\dA-F]{4})\s+U\+([\dA-F]{4}).*')
  MIN_LO = 0x40
  ELEMS_PER_ROW = 16
  
  def __init__(self, stream):
    self._codes = {}
    for line in stream:
      props = ShiftJIS.LINEFORMAT.match(line)
      if props:
        jis = int(props.group(1), 16)
        uni = int(props.group(2), 16)
        self._codes[jis] = uni
        #print("0x%04x -> 0x%04x" % (jis, uni), file = sys.stderr)

  def dump(self, stream):
    print("static const uint_t MIN_LO = 0x%x;" % ShiftJIS.MIN_LO, file = stream)
    self._dumpTable(0x81, 0xa0, stream)
    self._dumpTable(0xe0, 0xfd, stream)
    for jis in self._codes:
      uni = self._codes.get(jis)
      print("0x%04x -> 0x%04x" % (jis, uni), file = sys.stderr)
    if 0 != len(self._codes):
      raise Exception("Not all jis codes are covered by ranges")
    
  def _dumpTable(self, min, max, stream):
    print("static const uint16_t SJIS2UNICODE_%X[] = {" % min, file = stream, end = '')
    for hi in range(min, max):
      self._dumpRow(hi, stream);
    print ("\n};\n", file = stream)
   
  def _dumpRow(self, hi, stream):
    print("\n  //hi=0x%02x" % hi, file = stream, end = '');
    for lo in range(ShiftJIS.MIN_LO, 0x100):
        pair = 256 * hi + lo
        uni = self._codes.pop(pair, 0);
        if 0 == lo % ShiftJIS.ELEMS_PER_ROW:
          print("\n  ", file = stream, end = '')
        print("0x%04x, " % uni, file = stream, end = '')

def main():
  jis = ShiftJIS(sys.stdin)
  jis.dump(sys.stdout)

if __name__ == '__main__':
  main()
