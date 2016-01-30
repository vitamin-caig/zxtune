/**
 * 
 * @file
 *
 * @brief Substitution for android.os.CancellationSignal from APIv16
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.ui.browser;

//TODO: use from android.os when api16 will be minimal
class CancellationSignal {

  private volatile boolean canceled;

  final void cancel() {
    canceled = true;
  }

  final void throwIfCanceled() {
    if (canceled) {
      throw new OperationCanceledException();
    }
  }
}
  
class OperationCanceledException extends RuntimeException {
  private static final long serialVersionUID = 1L;
}
