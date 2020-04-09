package app.zxtune.fs.provider;

import android.database.Cursor;

import java.util.concurrent.Callable;

interface AsyncQueryOperation extends Callable<Cursor> {

  Cursor status();
}
