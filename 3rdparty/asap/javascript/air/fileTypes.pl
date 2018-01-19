for (qw(sap cmc cm3 cmr cms dmc dlt mpt mpd rmt tmc tm8 tm2 fc)) {
	print qq{		<fileType>
			<name>$_.file</name>
			<extension>$_</extension>
			<description>Atari 8-bit music</description>
			<contentType>application/x-$_</contentType>
		</fileType>
};
}
