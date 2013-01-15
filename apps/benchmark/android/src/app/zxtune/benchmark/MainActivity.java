package app.zxtune.benchmark;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Build;
import android.view.View;
import android.widget.Button;
import android.widget.ListView;
import android.widget.ArrayAdapter;
import android.util.Log;
import java.util.ArrayList;
import app.zxtune.benchmark.Benchmark;

public class MainActivity extends Activity {

  static final private String TAG = "zxtune.benchmark";

  private TestsTask Task;
  private ArrayAdapter<String> Report;
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main);

    if (Report == null) {
      final ArrayList<String> reportData = new ArrayList<String>();
      Report = new ArrayAdapter<String>(this, android.R.layout.test_list_item, reportData);
      Report.setNotifyOnChange(true);
    }
    final ListView view = (ListView)findViewById(R.id.test_results);
    view.setAdapter(Report);
  }

  public void onClick(View v) {
    switch (v.getId()) {
    case R.id.test_start:
      Task = new TestsTask();
      Task.execute();
      break;
    case R.id.app_exit:
      if (Task != null) {
        Task.cancel(false);
      }
      finish();
      break;
    }
  }

  private class TestsTask extends AsyncTask<Void, String, Void> {

    private String LastCategory;

    @Override
    protected void onPreExecute() {
      super.onPreExecute();
      ((Button)findViewById(R.id.test_start)).setEnabled(false);
      Report.clear();
      try {
        final PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
        Report.add(String.format("Benchmark %s (%d)", info.versionName, info.versionCode));
      }
      catch (NameNotFoundException e) {
        Report.add("Unknown version of program!");
      }
      Report.add(String.format("Android %s (%s-%s)", Build.VERSION.RELEASE, Build.VERSION.INCREMENTAL, Build.VERSION.CODENAME));
      Report.add(String.format("Device %s (%s/%s)", Build.MODEL, Build.CPU_ABI, Build.CPU_ABI2));
    }

    @Override
    protected Void doInBackground(Void... params) {
      Benchmark.ForAllTests(new TestsVisitor());
      return null;
    }

    @Override
    protected void onProgressUpdate(String... values) {
      super.onProgressUpdate(values);
      Report.add(values[0]);
    }

    @Override
    protected void onPostExecute(Void result) {
      super.onPostExecute(result);
      ((Button)findViewById(R.id.test_start)).setEnabled(true);
    }

    private class TestsVisitor implements Benchmark.TestVisitor {

      public boolean OnTest(String category, String name) {
        return !isCancelled();
      }

      public void OnTestResult(String category, String name, double res) {
        Log.d(TAG, String.format("Finished test (%s : %s): %f", category, name, res));
        if (LastCategory == null || !LastCategory.equals(category)) {
          publishProgress(category);
          LastCategory = category;
        }
        publishProgress(String.format("  %s: x%f", name, res));
      }
    }
  }
}
