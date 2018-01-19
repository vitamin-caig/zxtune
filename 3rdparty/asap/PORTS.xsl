<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" />

	<xsl:template match="/ports">
		<html>
			<head>
				<title>ASAP ports list</title>
				<style>
					table { border-collapse: collapse; }
					th, td { border: solid black 1px; }
					th, .name { background-color: #ccf; }
					.good { background-color: #cfc; }
					.bad { background-color: #fcc; }
					.partial { background-color: #ffc; }
					.good, .bad, .partial { text-align: center; }
				</style>
			</head>
			<body>
				<table>
					<thead>
						<tr>
							<th>Name</th>
							<th>Binary release</th>
							<th>Platform</th>
							<th>User interface</th>
							<th>First appeared in&#160;ASAP</th>
							<th>Develop&#173;ment status<sup><a href="#status_note">[1]</a></sup></th>
							<th>Output</th>
							<th>Supports sub&#173;songs?</th>
							<th>Shows file infor&#173;mation?</th>
							<th>Edits file infor&#173;mation?</th>
							<th>Converts to and from SAP?</th>
							<th>Configu&#173;rable play&#173;back time?</th>
							<th>Mute POKEY chan&#173;nels?</th>
							<th>Shows STIL?</th>
							<th>Comment</th>
							<th>Program&#173;ming lan&#173;guage</th>
							<th>Related website</th>
						</tr>
					</thead>
					<tbody>
						<xsl:apply-templates />
					</tbody>
				</table>
				<ol>
					<li id="status_note">Development status:
						<ul>
							<li><span class="good">stable</span> - complete</li>
							<li><span class="partial">in development</span> - working, but incomplete or buggy</li>
							<li><span class="bad">experimental</span> - initial implementation</li>
							<li><span class="bad">sample</span> - sample code for developers, not recommended for end-users</li>
						</ul>
					</li>
				</ol>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="port">
		<tr>
			<td class="name"><xsl:value-of select="@name" /></td>
			<td><xsl:value-of select="bin" /></td>
			<td><xsl:value-of select="platform" /></td>
			<td><xsl:value-of select="interface" /></td>
			<td><xsl:value-of select="since" /></td>
			<td>
				<xsl:attribute name="class">
					<xsl:choose>
						<xsl:when test="status = 'stable'">good</xsl:when>
						<xsl:when test="status = 'experi&#173;mental' or status = 'sample'">bad</xsl:when>
						<xsl:otherwise>partial</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
				<xsl:value-of select="status" />
			</td>
			<td><xsl:value-of select="output" /></td>
			<td><xsl:apply-templates select="subsongs" /></td>
			<td><xsl:apply-templates select="file-info" /></td>
			<td><xsl:apply-templates select="edit-info" /></td>
			<td><xsl:apply-templates select="convert-sap" /></td>
			<td><xsl:apply-templates select="config-time" /></td>
			<td><xsl:apply-templates select="mute-pokey" /></td>
			<td><xsl:apply-templates select="stil" /></td>
			<td><xsl:apply-templates select="comment" /></td>
			<td><xsl:value-of select="lang" /></td>
			<td><xsl:copy-of select="a" /></td>
		</tr>
	</xsl:template>

	<xsl:template match="subsongs|file-info|edit-info|convert-sap|config-time|mute-pokey|stil|comment">
		<xsl:attribute name="class">
			<xsl:choose>
				<xsl:when test="@class"><xsl:value-of select="@class" /></xsl:when>
				<xsl:when test="starts-with(., 'yes')">good</xsl:when>
				<xsl:when test=". = 'no'">bad</xsl:when>
				<xsl:otherwise>partial</xsl:otherwise>
			</xsl:choose>
		</xsl:attribute>
		<xsl:value-of select="." />
	</xsl:template>
</xsl:stylesheet>
