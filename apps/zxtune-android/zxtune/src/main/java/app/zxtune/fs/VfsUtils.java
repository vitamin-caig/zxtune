package app.zxtune.fs;

import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

public final class VfsUtils {

  @DrawableRes
  @Nullable
  public static Integer getObjectIcon(VfsObject obj) {
    final Icon annotation = obj.getClass().getAnnotation(Icon.class);
    if (annotation != null) {
      return annotation.value();
    }
    return VfsExtensionsKt.getIcon(obj);
  }
}


@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
@interface Icon {
  int value();
}
