/*
 * binaryHttpRequest.js - download binary file from JavaScript
 *
 * Copyright (C) 2009-2012  Piotr Fusik
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

var binaryHttpRequestIEHack = false;

if (/msie/i.test(navigator.userAgent) && !/opera/i.test(navigator.userAgent)) {
	document.write('<script type="text/vbscript">\n\
		Function fromVBArray(vbArray)\n\
			Dim i\n\
			ReDim jsArray(LenB(vbArray) - 1)\n\
			For i = 1 To LenB(vbArray)\n\
				jsArray(i - 1) = AscB(MidB(vbArray, i, 1))\n\
			Next\n\
			fromVBArray = jsArray\n\
		End Function\n\
	</script>');
	binaryHttpRequestIEHack = true;
}

function binaryHttpRequest(url, onload)
{
	var req;
	if (window.XMLHttpRequest)
		req = new XMLHttpRequest(); // Chrome/Mozilla/Safari/IE7+
	else
		req = new ActiveXObject("Microsoft.XMLHTTP"); // IE6
	req.open("GET", url, true);
	if (req.overrideMimeType)
		req.overrideMimeType("text/plain; charset=x-user-defined");
	else
		req.setRequestHeader("Accept-Charset", "x-user-defined");
	req.onreadystatechange = function() {
		if (req.readyState == 4 && (req.status == 200 || req.status == 0)) {
			var result;
			if (binaryHttpRequestIEHack)
				result = fromVBArray(req.responseBody).toArray();
			else {
				var response = req.responseText;
				result = new Array(response.length);
				for (var i = 0; i < response.length; i++)
					result[i] = response.charCodeAt(i) & 0xff;
			}
			onload(url, result);
		}
	};
	req.send(null);
}
