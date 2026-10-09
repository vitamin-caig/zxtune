# Classes reached by FindClass/descriptor/by-name throw are pinned with
# @androidx.annotation.Keep in the Kotlin sources (checked by JniContractTest).
# Classes that only export native methods (JniApi, JniOptions) are already pinned
# by AGP's default -keepclasseswithmembernames ... native <methods>.

# Reached through a JNI descriptor, so only the name matters, which @Keep cannot express
-keepnames class app.zxtune.core.Module

# Room's own consumer rule only protects the class names of the generated *_Impl
# databases. Their no-arg constructors are reached reflectively through
# Room.getGeneratedImplementation -> Class.forName on a computed name, which R8
# cannot resolve, so they get pruned. Losing *_Impl.<init> prunes
# RoomDatabase.<init> as well, leaving RoomDatabase.invalidationTracker with no
# writer: every read folds to const null and
# ThrowCatchOptimizer.optimizeAlwaysThrowingInstructions replaces
# InvalidationTracker.syncTriggers() (an always-null receiver) with `throw null`,
# which then collapses internalBeginTransaction(), beginTransaction(),
# runInTransaction() and every Room-backed updateCache() along with them.
-keep class * extends androidx.room.RoomDatabase { <init>(); }

-keepattributes SourceFile, LineNumberTable, *Annotation*

-assumenosideeffects class app.zxtune.Log { public static void d(...); }

# AGP already dumps mapping/usage/configuration per variant into
# pkg/zxtune/outputs/mapping/<variant>/, and r8Report* derives the removal report
# from those. Adding -printmapping/-printconfiguration here would only write
# duplicate copies into the module directory, where variants clobber each other.
