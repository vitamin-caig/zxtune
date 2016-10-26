package app.zxtune;

import android.content.Context;

import com.crashlytics.android.Crashlytics;
import com.crashlytics.android.answers.Answers;
import com.crashlytics.android.ndk.CrashlyticsNdk;

import io.fabric.sdk.android.Fabric;

public class Analytics {

  private static final String TAG = Analytics.class.getName();

  public static void initialize(Context ctx) {
    Fabric.with(ctx, new Crashlytics(), new CrashlyticsNdk(), new Answers());
  }
}
