/**
 * @file
 * @brief Fragment for storing CallbackSubscription
 * @version $Id:$
 * @author
 */
package app.zxtune;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.util.Log;
import app.zxtune.playback.Callback;
import app.zxtune.playback.CallbackSubscription;
import app.zxtune.playback.CompositeCallback;
import app.zxtune.rpc.RemoteSubscription;

public class RetainedCallbackSubscriptionFragment extends Fragment implements CallbackSubscription {

  private static final String TAG = RetainedCallbackSubscriptionFragment.class.getName();
  private final CompositeCallback receivers;
  private Releaseable connection;
  
  public RetainedCallbackSubscriptionFragment() {
    Log.d(TAG, "Create instance");
    this.receivers = new CompositeCallback();
  }
  
  public static void register(FragmentManager manager, FragmentTransaction transaction) {
    if (find(manager) == null) {
      final Fragment self = new RetainedCallbackSubscriptionFragment();
      transaction.add(self, TAG);
    }
  }
  
  public static CallbackSubscription find(FragmentManager manager) {
    return (RetainedCallbackSubscriptionFragment) manager.findFragmentByTag(TAG);
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    setRetainInstance(true);
  }
  
  @Override
  public void onAttach(Activity activity) {
    super.onAttach(activity);
    
    assert connection == null;
    final Intent intent = new Intent(activity, PlaybackService.class);
    final CallbackSubscription subscription = new RemoteSubscription(activity, intent);
    connection = subscription.subscribe(receivers);
  }
  
  @Override
  public void onDetach() {
    super.onDetach();
    
    try {
      if (connection != null) {
        connection.release();
      }
    } finally {
      connection = null;
    }
  }
  
  @Override
  public Releaseable subscribe(Callback callback) {
    return receivers.subscribe(callback);
  }
}
