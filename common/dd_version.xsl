<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xs="http://www.w3.org/2001/XMLSchema" version="1.0">

<!-- XSL transformation for getting the version of the Data Dictionary -->

<xsl:output method="text" />

<xsl:template match="/IDSs">
  <xsl:for-each select="version">
    <xsl:value-of select="text()"/>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
