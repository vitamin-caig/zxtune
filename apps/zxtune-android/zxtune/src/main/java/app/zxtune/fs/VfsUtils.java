package app.zxtune.fs;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

public final class VfsUtils {

  public static int getObjectIcon(VfsObject obj) {
    final Icon annotation = obj.getClass().getAnnotation(Icon.class);
    if (annotation != null) {
      return annotation.value();
    }
    return 0;
  }
}


@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
@interface Icon {
  int value();
}
