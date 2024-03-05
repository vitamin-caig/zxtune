package app.zxtune.core

interface ModuleDetectCallback {
    fun onModule(id: Identifier, obj: Module)
}
