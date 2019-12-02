#!/usr/bin/python3

import re
import subprocess

def getDeps(target):
  print('Targets for ' + target)
  proc = subprocess.run(['./gradlew', target, '--dry-run'], stdout=subprocess.PIPE)
  result = set()
  for line in proc.stdout.decode('utf-8').split():
    if line.startswith(':zxtune:'):
      result.add(line[8:])
  return result

def checkDeps(descr):
  deps = getDeps(descr['target'])
  print(' Missing:')
  for req in descr['required']:
    found = False
    for d in deps:
      if re.search(req, d):
        found = True
        break
    if not found:
      print('  ' + req)
  print(' Redundand:')
  for req in descr['excluded']:
    for d in deps:
      if re.search(req, d):
        print('  ' + d)

DEPS = [
    {
      'target': 'publicBuildFatRelease',
      'required': [ 'publishApk.*', 'assemble.*' ],
      'excluded': [ 'publishAab*.', 'bundle.*' ]
    },
    {
      'target': 'publicBuildWithCrashlytics',
      'required': [ 'publishApkFatRelease', 'publishAabThinRelease', 'publishPdb', 'prepareDebugSymbols.*'],
      'excluded': [ '.*bundleFat.*', '.*assembleThin.*']
    }
]

for d in DEPS:
  checkDeps(d)
