/*
 * @file
 * @brief Phone calls handler
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.content.Context;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import app.zxtune.playback.PlaybackControl;

public class PhoneCallHandler extends PhoneStateListener {
    
  private static final String TAG = PhoneCallHandler.class.getName();
  private final TelephonyManager manager;
  private final PlaybackControl control;
  private boolean playedOnCall;
  
  public PhoneCallHandler(Context context, PlaybackControl control) {
    this.manager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
    this.control = control;
  }
  
  public void register() {
    Log.d(TAG, "Registered");
    manager.listen(this, PhoneStateListener.LISTEN_CALL_STATE);
  }
  
  public void unregister() {
    manager.listen(this, PhoneStateListener.LISTEN_NONE);
    Log.d(TAG, "Unregistered");
  }
  
  @Override
  public void onCallStateChanged(int state, String incomingNumber) {
    Log.d(TAG, "Process call state to " + state);
    switch (state) {
      case TelephonyManager.CALL_STATE_RINGING://incoming call
        processCall();
        break;
      case TelephonyManager.CALL_STATE_OFFHOOK://outgoing call
        processCall();
        break;
      case TelephonyManager.CALL_STATE_IDLE://hangout
        processIdle();
        break;
    }
  }
  
  private void processCall() {
    if (playedOnCall = control.isPlaying()) {
      control.stop();
    }
  }
  
  private void processIdle() {
    if (playedOnCall && !control.isPlaying()) {
      playedOnCall = false;
      control.play();
    }
  }
}
