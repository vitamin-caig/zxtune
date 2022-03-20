package app.zxtune.fs.vgmrips

import app.zxtune.TimeStamp

data class Track(
  //1-based
  val number: Int,
  val title: String,
  val duration: TimeStamp,
  //unescaped path after /packs/vgm/
  val location: String
)
