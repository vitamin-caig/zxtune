/**
 *
 * @file
 *
 * @brief Handling phone call events
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.content.Context;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;

import app.zxtune.playback.PlaybackControl;

class PhoneCallHandler extends PhoneStateListener {
  
  private static final String TAG = PhoneCallHandler.class.getName();
    
  private final PlaybackControl control;
  private boolean playedOnCall;
  
  private PhoneCallHandler(PlaybackControl control) {
    this.control = control;
  }
  
  public static Releaseable subscribe(Context context, PlaybackControl control) {
    final TelephonyManager manager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
    final PhoneStateListener listener = new PhoneCallHandler(control);
    return new Connection(manager, listener);
  }
  
  @Override
  public void onCallStateChanged(int state, String incomingNumber) {
    Log.d(TAG, "Process call state to %d", state);
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
  
  private static class Connection implements Releaseable {

    private final TelephonyManager manager;
    private final PhoneStateListener listener;
    
    Connection(TelephonyManager manager, PhoneStateListener listener) {
      this.manager = manager;
      this.listener = listener;
      manager.listen(listener, PhoneStateListener.LISTEN_CALL_STATE);
      Log.d(listener.getClass().getName(), "Registered");
    }
    
    @Override
    public void release() {
      manager.listen(listener, PhoneStateListener.LISTEN_NONE);
      Log.d(listener.getClass().getName(), "Unregistered");
    }
  }
}
