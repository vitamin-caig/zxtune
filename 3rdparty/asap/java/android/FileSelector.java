/*
 * FileSelector.java - ASAP for Android
 *
 * Copyright (C) 2010-2014  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

package net.sf.asap;

import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import java.io.InputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collection;
import java.util.TreeSet;

public class FileSelector extends ListActivity
{
	private boolean isSearch;
	private boolean isDetails;
	private Uri uri;

	private static class FileInfo implements Comparable<FileInfo>
	{
		private String filename;
		private String title;
		private String author;
		private String date;
		private int songs;

		private FileInfo(String filename)
		{
			this.title = this.filename = filename;
		}

		private FileInfo(String filename, InputStream is) throws Exception
		{
			this(filename);
			if (is != null) {
				byte[] module = new byte[ASAPInfo.MAX_MODULE_LENGTH];
				int moduleLen = Util.readAndClose(is, module);
				ASAPInfo info = new ASAPInfo();
				info.load(filename, module, moduleLen);
				this.title = info.getTitleOrFilename();
				this.author = info.getAuthor();
				this.date = info.getDate();
				this.songs = info.getSongs();
			}
		}

		private FileInfo(String filename, String title)
		{
			this.filename = filename;
			this.title = title;
		}

		@Override
		public String toString()
		{
			return title;
		}

		@Override
		public boolean equals(Object obj)
		{
			if (!(obj instanceof FileInfo))
				return false;
			FileInfo that = (FileInfo) obj;
			if (this.filename == null)
				return that.filename == null;
			return this.filename.equals(that.filename);
		}

		@Override
		public int hashCode()
		{
			return filename == null ? 0 : filename.hashCode();
		}

		public int compareTo(FileInfo that)
		{
			if (this.filename == null)
				return -1;
			if (that.filename == null)
				return 1;
			boolean dir1 = this.filename.endsWith("/");
			boolean dir2 = that.filename.endsWith("/");
			if (dir1 != dir2)
				return dir1 ? -1 : 1;
			int titleCmp = this.title.compareTo(that.title);
			if (titleCmp != 0)
				return titleCmp;
			return this.filename.compareTo(that.filename);
		}
	}

	private static class FileInfoAdapter extends ArrayAdapter<FileInfo>
	{
		private LayoutInflater layoutInflater;

		private FileInfoAdapter(Context context, int rowViewResourceId, FileInfo[] infos)
		{
			super(context, rowViewResourceId, infos);
			layoutInflater = LayoutInflater.from(context);
		}

		private static class ViewHolder
		{
			private TextView title;
			private TextView author;
			private TextView date;
			private TextView songs;
		}

		public View getView(int position, View convertView, ViewGroup parent)
		{
			ViewHolder holder;
			if (convertView == null) {
				convertView = layoutInflater.inflate(R.layout.fileinfo_list_item, null);
				holder = new ViewHolder();
				holder.title = (TextView) convertView.findViewById(R.id.title);
				holder.author = (TextView) convertView.findViewById(R.id.author);
				holder.date = (TextView) convertView.findViewById(R.id.date);
				holder.songs = (TextView) convertView.findViewById(R.id.songs);
				convertView.setTag(holder);
			}
			else
				holder = (ViewHolder) convertView.getTag();

			FileInfo info = getItem(position);
			holder.title.setText(info.title);
			holder.author.setText(info.author);
			holder.date.setText(info.date);
			holder.songs.setText(info.songs > 1 ? getContext().getString(R.string.songs_format, info.songs) : null);

			return convertView;
		}
	}

	private class FileInfoList extends FileContainer
	{
		private Collection<FileInfo> coll;
		private int songFiles;

		@Override
		protected void onSongFile(String name, InputStream is) throws Exception
		{
			coll.add(new FileInfo(name, is));
			songFiles++;
		}

		@Override
		protected void onContainer(String name)
		{
			coll.add(new FileInfo(name));
		}

		FileInfo[] list() throws IOException
		{
			boolean isM3u = Util.endsWithIgnoreCase(uri.toString(), ".m3u");
			coll = isM3u ? new ArrayList<FileInfo>() : new TreeSet<FileInfo>();
			songFiles = 0;
			list(uri, isDetails, false);

			// "(shuffle all)" if any song files or non-empty ZIP directory
			if (songFiles > 1 || (!coll.isEmpty() && Util.endsWithIgnoreCase(uri.getPath(), ".zip"))) {
				FileInfo shuffleAll = new FileInfo(null, getString(R.string.shuffle_all));
				if (isM3u)
					((ArrayList<FileInfo>) coll).add(0, shuffleAll); // insert at the beginning
				else
					coll.add(shuffleAll);
			}

			return coll.toArray(new FileInfo[coll.size()]);
		}
	}

	private void reload()
	{
		uri = getIntent().getData();
		if (uri == null)
			uri = Uri.parse("file:///");

		String title = uri.getPath();
		String fragment = uri.getFragment();
		if (fragment != null)
			title += "#" + fragment;
		setTitle(getString(R.string.selector_title, title));

		FileInfo[] infos;
		try {
			infos = new FileInfoList().list();
		}
		catch (IOException ex) {
			Toast.makeText(this, R.string.access_denied, Toast.LENGTH_SHORT).show();
			infos = new FileInfo[0];
		}

		ListAdapter adapter = isDetails
			? new FileInfoAdapter(this, R.layout.fileinfo_list_item, infos)
			: new ArrayAdapter<FileInfo>(this, R.layout.filename_list_item, infos);
		setListAdapter(adapter);
	}

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		getListView().setTextFilterEnabled(true);
		isDetails = getPreferences(MODE_PRIVATE).getBoolean("fileDetails", false);
		reload();
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		Intent intent;
		FileInfo info = (FileInfo) l.getItemAtPosition(position);
		String name = info.filename;
		if (name == null) {
			// shuffle all
			intent = new Intent(Intent.ACTION_VIEW, uri, this, Player.class);
		}
		else {
			Class klass = ASAPInfo.isOurFile(name) ? Player.class : FileSelector.class;
			intent = new Intent(Intent.ACTION_VIEW, Util.buildUri(uri, name), this, klass);
			if (Util.endsWithIgnoreCase(uri.toString(), ".m3u"))
				intent.putExtra(PlayerService.EXTRA_PLAYLIST, uri.toString());
		}
		startActivity(intent);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		getMenuInflater().inflate(R.menu.file_selector, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId()) {
		case R.id.menu_search:
			InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
			if (isSearch) {
				imm.hideSoftInputFromWindow(getListView().getWindowToken(), 0);
				getListView().clearTextFilter();
				isSearch = false;
			}
			else {
				imm.showSoftInput(getListView(), 0);
				isSearch = true;
			}
			return true;
		case R.id.menu_toggle_details:
			isDetails = !isDetails;
			getPreferences(MODE_PRIVATE).edit().putBoolean("fileDetails", isDetails).commit();
			reload();
			return true;
		case R.id.menu_about:
			Util.showAbout(this);
			return true;
		default:
			return false;
		}
	}
}
