<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:yaslt="http://www.mod-xslt2.com/ns/1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xs="http://www.w3.org/2001/XMLSchema" version="2.0" extension-element-prefixes="yaslt" xmlns:fn="http://www.w3.org/2005/02/xpath-functions" xmlns:local="http://www.example.com/functions/local" exclude-result-prefixes="local xs">
  <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>
  <!-- This script transforms the IDSDef.xml file into an XML file which will be used to build the MDS+ tree-->
  <!-- It contains the full data structure and describes it in terms of MDS+ terminology (node, member) and types (TEXT,NUMERIC,SIGNAL)-->
  <!-- This XML file has then to be processed by the Java routine CompileTree to create the MDS+ tree (part of the MDS+ libraries) -->
  <!-- Time dependent numeric signals are of the SIGNAL type -->
  <!-- Time independent numeric signals are of the NUMERIC type -->
  <!-- Only Vector of strings (str_1d_type) are of the TEXT type --> <!-- Not sure the vector of strings with time-dependence will work with a TEXT declaration, though -->
  <!-- Written by F. Imbeaux -->

  <!-- Scan for top-level elements -->
  <xsl:template match = "/*">
    <tree>
    <member NAME="REF_INFO" USAGE="NUMERIC"/>
      <xsl:apply-templates select = "IDS"/>
    </tree>
  </xsl:template>

  <!-- First, we scan at the IDS level -->
  <xsl:template match = "IDS">
    <node>
          <xsl:attribute name="NAME"><xsl:value-of select="@name"/></xsl:attribute>
      <xsl:apply-templates select = "field"/>

      <!-- Create subtrees for IDS occurrence. If parameter maxoccur is not defined, 10 additional occurrences are created-->
      <xsl:call-template name = "addOccurrence" >
        <xsl:with-param name = "iteration" select = "if (@maxoccur) then @maxoccur else 10" />
      </xsl:call-template>

  </node>
  </xsl:template>



  <!-- Then we scan the fields recursively -->
  <xsl:template match = "field">
    <xsl:choose>
<!--      <xsl:when test="@data_type='structure'">    -->
      <xsl:when test="@data_type='structure' or @data_type='struct_array'" >
        <!-- this is a structure : write as node and scan the child elements recursively -->
        <node>
          <xsl:attribute name="NAME"><xsl:value-of select="@name"/></xsl:attribute>

          <xsl:apply-templates select = "field"/>
        </node>
      </xsl:when>
      <xsl:otherwise>
        <!-- this is a bottom element -->
        <member>
          <xsl:attribute name="NAME"><xsl:value-of select="@name"/></xsl:attribute>
          <xsl:choose>
            <xsl:when test="@data_type='str_1d_type' or @data_type='STR_1D' and @type!='dynamic'">
              <xsl:attribute name="USAGE">NUMERIC</xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
              <xsl:choose>
                <xsl:when test="@type='dynamic'">
                  <xsl:attribute name="USAGE">SIGNAL</xsl:attribute>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:attribute name="USAGE">NUMERIC</xsl:attribute>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:otherwise>
          </xsl:choose>
        </member></xsl:otherwise>
      </xsl:choose>
    </xsl:template>

  
  <!-- Additional IDS occurrence creation -->
  <xsl:template name="addOccurrence">
    <xsl:param name="iteration"/>
    <xsl:variable name="current" select="."/>
    <xsl:for-each select="1 to $iteration">
      <node>
        <xsl:attribute name="NAME">
          <xsl:value-of select="."/>
        </xsl:attribute>
        <xsl:apply-templates select = "$current/field"/>
      </node>
    </xsl:for-each>
  </xsl:template>

  </xsl:stylesheet>
