package app.zxtune.benchmark;

import android.app.Activity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ListView;
import android.widget.ArrayAdapter;
import android.util.Log;
import java.util.ArrayList;
import app.zxtune.benchmark.Benchmark;

public class MainActivity extends Activity {

  static final private String TAG = "zxtune.benchmark";
  static final private int START_ID = Menu.FIRST;
  static final private int EXIT_ID = START_ID + 1;
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.main);
  }

  public boolean onCreateOptionsMenu(Menu menu) {
    super.onCreateOptionsMenu(menu);

    menu.add(0, START_ID, 0, R.string.start);
    menu.add(0, EXIT_ID, 0, R.string.exit);
    return true;
  }

  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
    case START_ID:
      StartTests();
      return true;
    case EXIT_ID:
      finish();
      return true;
    }
    return super.onOptionsItemSelected(item);
  }

  private void StartTests() {
    ListView view = (ListView)findViewById(R.id.test_results);
    ArrayList<String> res = new ArrayList<String>();
    ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.test_list_item, res);
    adapter.setNotifyOnChange(true);
    view.setAdapter(adapter);
    Benchmark.ForAllTests(new AllTestsVisitor(res));
  }

  private class AllTestsVisitor implements Benchmark.TestVisitor {
  
    private String LastCategory;
    private ArrayList<String> Result;
    
    AllTestsVisitor(ArrayList<String> result) {
      Result = result;
    }
    
    public boolean OnTest(String category, String name) {
      Log.d(TAG, String.format("Enabling test (%s : %s)", category, name));
      return true;
    }
    
    public void OnTestResult(String category, String name, double res) {
      Log.d(TAG, String.format("Finished test (%s : %s): %f", category, name, res));
      if (LastCategory == null || !LastCategory.equals(category)) {
        Result.add(category);
        LastCategory = category;
      }
      Result.add(String.format("  %s: x%f", name, res));
    }
  }
}
