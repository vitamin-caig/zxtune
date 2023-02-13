import os
import sys

class Fs(object):
  @staticmethod
  def list_files(paths):
    queue = list()
    queue.extend(paths)
    while queue:
      to_scan = queue.pop(0)
      if os.path.isfile(to_scan):
        yield to_scan
      else:
        for entry in os.scandir(to_scan):
          full_path = entry.path
          if entry.is_dir():
            queue.append(full_path)
          elif entry.is_file():
            yield full_path

class Sources(object):
  def __init__(self):
    self._tags = dict()

  def try_add(self, path):
    if path.endswith(('.h', '.cpp')):
      self._add(Sources._normalize(path))

  def try_find(self, tag):
    for (file_tag, path) in self._tags.items():
      if tag >= file_tag and tag - file_tag < 65535:
        return (path, tag - file_tag)
    return (None, None)

  def _add(self, path):
    tag = Sources._get_tag(path)
    if tag in self._tags:
      raise Exception('Duplicated entries for tag {}: {} and {}'.format(tag, path, self._tags[tag]))
    self._tags[tag] = path

  @staticmethod
  def _normalize(path):
    return path.replace('\\', '/').lower()

  @staticmethod
  def _get_tag(path):
    res = 5381
    for c in path.encode():
      res = (res * 33 + c) & 0xffffffff
    return res

def fill_tags(dirs):
  if len(dirs) < 1:
    raise Exception('No dirs to find')
  result = Sources()
  for src in Fs.list_files(dirs):
    result.try_add(src)
  return result

if __name__ == '__main__':
  tags = fill_tags(['src', 'include', 'apps'])
  for tag in sys.argv[1:]:
    (path, line) = tags.try_find(int(tag, 10))
    if path:
      print('{} = {}:{}'.format(tag, path, line))
    else:
      print('{} not found!'.format(tag))
