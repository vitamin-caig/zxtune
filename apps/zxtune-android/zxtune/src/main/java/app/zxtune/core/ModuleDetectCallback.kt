package app.zxtune.core

interface ModuleDetectCallback {
  fun onModule(subpath: String, obj: Module)
}
