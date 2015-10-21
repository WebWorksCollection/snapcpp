<?xml version="1.0" encoding="utf-8"?>
<!--

This sitemap XSL processor was taken from Drupal.org

Copyright (c) 2010  Dave Reid <http://drupal.org/user/53892>
Copyright (c) 2015  Alexis Wilke <http://snapwebsites.org/>

This file is free software: you may copy, redistribute and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 2 of the License, or (at your
option) any later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

This file incorporates work covered by the following copyright and
permission notice:

Google Sitmaps Stylesheets (GSStylesheets)
Project Home: http://sourceforge.net/projects/gstoolbox
Copyright (c) 2005 Baccou Bonneville SARL (http://www.baccoubonneville.com)
License http://www.gnu.org/copyleft/lesser.html GNU/LGPL

-->
<xsl:stylesheet version="2.0"
    xmlns:html="http://www.w3.org/TR/REC-html40"
    xmlns:sitemap="http://www.sitemaps.org/schemas/sitemap/0.9"
    xmlns:image="http://www.google.com/schemas/sitemap-image/1.1"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" version="1.0" encoding="utf-8" indent="yes"/>
  <!-- Root template -->
  <xsl:template match="/">
    <html>
      <head>
        <title>XML Sitemap</title>
      </head>

      <!-- Store in $fileType if we are in a sitemap or in a siteindex -->
      <xsl:variable name="fileType">
        <xsl:choose>
          <xsl:when test="//sitemap:url">sitemap</xsl:when>
          <xsl:otherwise>siteindex</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <body>
        <h1>Sitemap file</h1>
        <xsl:choose>
          <xsl:when test="$fileType='sitemap'"><xsl:call-template name="sitemapTable"/></xsl:when>
          <xsl:otherwise><xsl:call-template name="siteindexTable"/></xsl:otherwise>
        </xsl:choose>

        <div id="footer">
          <p>Generated by the <a href="http://snapwebsites.org/" target="_blank">Snap! Websites</a> core plugins.</p>
        </div>
      </body>
    </html>
  </xsl:template>

  <!-- siteindexTable template -->
  <xsl:template name="siteindexTable">
    <div id="information">
      <p>Number of sitemaps in this index: <xsl:value-of select="count(sitemap:sitemapindex/sitemap:sitemap)"></xsl:value-of></p>
    </div>
    <table class="tablesorter siteindex" border="1" cellpadding="3" cellspacing="0">
      <thead>
        <tr>
          <th>Sitemap URL</th>
          <th>Last modification date</th>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates select="sitemap:sitemapindex/sitemap:sitemap">
          <xsl:sort select="sitemap:lastmod" order="descending"/>
        </xsl:apply-templates>
      </tbody>
    </table>
  </xsl:template>

  <!-- sitemapTable template -->
  <xsl:template name="sitemapTable">
    <div id="information">
      <p>Number of URLs in this sitemap: <xsl:value-of select="count(sitemap:urlset/sitemap:url)"></xsl:value-of></p>
      <p>Total number of images in this sitemap: <xsl:value-of select="count(sitemap:urlset/sitemap:url/image:image)"></xsl:value-of></p>
    </div>
    <table class="tablesorter sitemap" border="1" cellpadding="3" cellspacing="0">
      <thead>
        <tr>
          <th>URL location</th>
          <th>Last modification date</th>
          <th>Change frequency</th>
          <th>Priority</th>
          <th>Images</th>
        </tr>
      </thead>
      <tbody>
        <xsl:apply-templates select="sitemap:urlset/sitemap:url">
          <xsl:sort select="sitemap:priority" order="descending"/>
        </xsl:apply-templates>
      </tbody>
    </table>
  </xsl:template>

  <!-- sitemap:url template -->
  <xsl:template match="sitemap:url">
    <tr>
      <td>
        <xsl:variable name="sitemapURL"><xsl:value-of select="sitemap:loc"/></xsl:variable>
        <a href="{$sitemapURL}" rel="nofollow"><xsl:value-of select="$sitemapURL"></xsl:value-of></a>
      </td>
      <td><xsl:value-of select="sitemap:lastmod"/></td>
      <td><xsl:value-of select="sitemap:changefreq"/></td>
      <td style="text-align: right;">
        <xsl:choose>
          <!-- If priority is not defined, show the default value of 0.5 -->
          <xsl:when test="sitemap:priority">
            <xsl:value-of select="sitemap:priority"/>
          </xsl:when>
          <xsl:otherwise>0.5</xsl:otherwise>
        </xsl:choose>
      </td>
      <td style="text-align: right;">
        <xsl:value-of select="count(image:image)"/>
      </td>
    </tr>
    <xsl:if test="count(image:image) > 0">
      <xsl:apply-templates select="image:image">
        <xsl:sort select="image:loc" order="ascending"/>
      </xsl:apply-templates>
    </xsl:if>
  </xsl:template>

  <!-- image:image template -->
  <xsl:template match="image:image">
    <tr>
      <td colspan="5">
        Image URL: <a href="{image:loc}"><xsl:value-of select="image:loc"/></a><br/>
        <xsl:if test="image:title">
          Image Title: <xsl:value-of select="image:title"/><br/>
        </xsl:if>
        <xsl:if test="image:caption">
          Image Caption: <xsl:value-of select="image:caption"/><br/>
        </xsl:if>
        <xsl:if test="image:geo_location">
          Image Geographic Location: <xsl:value-of select="image:geo_location"/><br/>
        </xsl:if>
        <xsl:if test="image:geo_location">
          Image License: <a href="{image:license}"><xsl:value-of select="image:license"/></a><br/>
        </xsl:if>
      </td>
    </tr>
  </xsl:template>

  <!-- sitemap:sitemap template -->
  <xsl:template match="sitemap:sitemap">
    <tr>
      <td>
        <xsl:variable name="sitemapURL"><xsl:value-of select="sitemap:loc"/></xsl:variable>
        <a href="{$sitemapURL}"><xsl:value-of select="$sitemapURL"></xsl:value-of></a>
      </td>
      <td><xsl:value-of select="sitemap:lastmod"/></td>
    </tr>
  </xsl:template>
</xsl:stylesheet>
<!--
vim: ts=2 sw=2 et
-->
