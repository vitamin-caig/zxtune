/*
 * @file
 * @brief Incoming calls handler
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.content.Context;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import app.zxtune.playback.Control;
import app.zxtune.playback.Status;

public class IncomingCallHandler extends PhoneStateListener {
    
  private static final String TAG = IncomingCallHandler.class.getName();
  private final TelephonyManager manager;
  private final Control control;
  private Status stateOnIncomingCall;
  
  public IncomingCallHandler(Context context, Control control) {
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
      case TelephonyManager.CALL_STATE_RINGING:
        processRinging();
        break;
      case TelephonyManager.CALL_STATE_OFFHOOK:
        processOffhook();
        break;
      case TelephonyManager.CALL_STATE_IDLE:
        processIdle();
        break;
    }
  }
  
  private void processRinging() {
    stateOnIncomingCall = control.getStatus();
    if (stateOnIncomingCall.equals(Status.PLAYING)) {
      control.pause();
    }
  }
  
  private void processOffhook() {
  }
  
  private void processIdle() {
    if (stateOnIncomingCall == null) {
      return;
    }
    final Status nowState = control.getStatus();
    if (!nowState.equals(Status.STOPPED) && stateOnIncomingCall.equals(Status.PLAYING)) {
      control.play();
    }
    stateOnIncomingCall = null;
  }
}
