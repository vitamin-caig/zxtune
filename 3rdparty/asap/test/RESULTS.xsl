<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" />

	<xsl:template match="/tests">
		<html>
			<head>
				<title>ASAP test results</title>
				<style>
					table { border-collapse: collapse; }
					th, td { border: solid black 1px; }
					th, .name { background-color: #ccf; }
					.pass { background-color: #cfc; }
					.fail { background-color: #fcc; }
					.pass, .fail { text-align: center; }
				</style>
			</head>
			<body>
				<table>
					<tr>
						<th>Test</th>
						<th>Atari800 2.2.1</th>
						<th>Atari++ 1.60</th>
						<th>Altirra 1.9</th>
						<th>ASAP 2.1.2</th>
						<th>ASAP 3.0.0</th>
					</tr>
					<xsl:apply-templates />
				</table>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="test">
		<tr>
			<td class="name"><xsl:value-of select="@name" /></td>
			<xsl:apply-templates select="*[@on='Atari800 2.2.1']" />
			<xsl:apply-templates select="*[@on='Atari++ 1.60']" />
			<xsl:apply-templates select="*[@on='Altirra 1.9']" />
			<xsl:apply-templates select="*[@on='ASAP 2.1.2']" />
			<xsl:apply-templates select="*[@on='ASAP 3.0.0']" />
		</tr>
	</xsl:template>

	<xsl:template match="pass">
		<td class="pass">Pass</td>
	</xsl:template>

	<xsl:template match="fail|crash">
		<td class="fail">FAIL</td>
	</xsl:template>
</xsl:stylesheet>
