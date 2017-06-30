# From https://github.com/krschultz/android-proguard-snippets

# Self app classes
-keep class app.zxtune.** { *; }
-keep class com.mobeta.android.dslv.** { *; }

# Crashlytics https://docs.fabric.io/android/crashlytics/dex-and-proguard.html
-keep class com.crashlytics.** { *; }
-dontwarn com.crashlytics.**
-keep public class * extends java.lang.Exception
-keepattributes SourceFile, LineNumberTable, *Annotation*

-dontobfuscate
