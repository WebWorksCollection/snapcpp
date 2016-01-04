<?xml version="1.0"?>
<!--
Snap Websites Server == timetracker form parser
Copyright (C) 2014-2016  Made to Order Software Corp.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
-->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
															xmlns:xs="http://www.w3.org/2001/XMLSchema"
															xmlns:fn="http://www.w3.org/2005/xpath-functions"
															xmlns:snap="snap:snap">

	<!-- some special variables to define the theme -->
	<xsl:variable name="layout-area">timetracker-parser</xsl:variable>
	<xsl:variable name="layout-modified">2016-01-02 05:32:33</xsl:variable>
	<xsl:variable name="layout-editor">timetracker-page</xsl:variable>

	<xsl:template match="snap">
		<output> <!-- lang="{$lang}" 'lang variable undefined' -->
			<div id="content" class="editor-form" form_name="timetracker">
				<xsl:attribute name="session"><xsl:copy-of select="page/body/editor/session/div/div/node()"/></xsl:attribute>

				<!-- xsl:if test="$action != 'edit' and $can_edit = 'yes'">
					<a class="settings-edit-button" href="?a=edit">Edit</a>
				</xsl:if>
				<xsl:if test="$action = 'edit'">
					<a class="settings-save-button" href="#">Save Changes</a>
					<a class="settings-cancel-button right-aligned" href="{/snap/head/metadata/desc[@type='page_uri']/data}">Cancel</a>
				</xsl:if-->
				<h3>Time Tracking</h3>
				<div>
					<xsl:attribute name="class">test<!--xsl:if test="$action = 'edit'"> editing</xsl:if--></xsl:attribute>

					<p>You are running Snap! v[version] (more <a href="/admin/versions?back=admin/settings/info">version information</a>)</p>
					<p>Site wide settings as offerred by the core system of your website.</p>

					<fieldset class="site-name">
						<legend>Site Name</legend>

						<!-- row -->
						<div class="editor-block">
							<label for="date" class="editor-title">Date:</label>
							<xsl:copy-of select="page/body/timetracker/date/node()"/>
						</div>
						<div class="editor-block">
							<label for="time" class="editor-title">Time:</label>
							<xsl:copy-of select="page/body/timetracker/time/node()"/>
						</div>
						<div class="editor-block">
							<label for="description" class="editor-title">Description:</label>
							<xsl:copy-of select="page/body/timetracker/description/node()"/>
						</div>

					</fieldset>

				</div>
			</div>
		</output>
	</xsl:template>
</xsl:stylesheet>
<!-- vim: ts=2 sw=2
-->
