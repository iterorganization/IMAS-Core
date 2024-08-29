<?xml version="1.0" encoding="UTF-8"?>
<?modxslt-stylesheet type="text/xsl" media="fuffa, screen and $GET[stylesheet]" href="./%24GET%5Bstylesheet%5D" alternate="no" title="Translation using provided stylesheet" charset="ISO-8859-1" ?>
<?modxslt-stylesheet type="text/xsl" media="screen" alternate="no" title="Show raw source of the XML file" charset="ISO-8859-1" ?>

<xsl:stylesheet 
   xmlns:yaslt="http://www.mod-xslt2.com/ns/1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
   xmlns:xs="http://www.w3.org/2001/XMLSchema" 
   xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
   xmlns:exsl="http://exslt.org/common"
   xmlns:str="http://exslt.org/strings"
   xmlns:func="http://exslt.org/functions"
   xmlns:my="http://localhost.localdomain/localns"
   exclude-result-prefixes="my"
   extension-element-prefixes="yaslt exsl func str">

<xsl:output method="text" version="1.0" encoding="UTF-8" indent="yes"/>

<!-- MAIN, FILE GENERATION -->
<xsl:template match="/constants">

  <!-- DOCBOOK FILE -->
  <exsl:document href="{$prefix}{$name}.xml" method="text">
    <xsl:call-template name="docbook"/>
  </exsl:document>

</xsl:template>

<!-- GENERIC FUNCTION TO HANDLE DESCRIPTION/UNIT -->
<func:function name="my:desc">
  <xsl:param name="symbol"/>
  <func:result>
    <xsl:if test="@description!='' or @units!=''">
      <xsl:text>  </xsl:text>
      <xsl:value-of select="$symbol"/>
      <xsl:text> </xsl:text>
    </xsl:if>
    <xsl:value-of select="@description"/>
    <xsl:if test="@units!=''"> [<xsl:value-of select="@units"/>]</xsl:if>
    <xsl:text>&#xA;</xsl:text>
  </func:result>
</func:function>


<!-- GENERIC FUNCTION TO UPPER CASE FIRST CHARACTER OF A STRING -->
<func:function name="my:upfirst">
  <xsl:param name="word"/>
  <func:result>
    <xsl:variable name="firstchar">
      <xsl:value-of select="translate(substring($word,1,1),'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
    </xsl:variable>
    <xsl:value-of select="concat($firstchar,substring($word,2))"/>
  </func:result>
</func:function>


<!-- GENERIC FUNCTION TO UPPER CASE ALL CHARACTERS OF A STRING -->
<func:function name="my:upall">
  <xsl:param name="word"/>
  <func:result>
    <xsl:value-of select="translate($word,'abcdefghijklmnopqrstuvwxyz','ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
  </func:result>
</func:function>


<!-- GENERIC TEMPLATE TO HANDLE MULTIPLE LINES COMMENTS -->
<xsl:template name="replace-string">
  <xsl:param name="text"/>
  <xsl:param name="replace"/>
  <xsl:param name="with"/>
  <xsl:choose>
    <xsl:when test="contains($text,$replace)">
      <xsl:value-of select="substring-before($text,$replace)"/>
      <xsl:value-of select="$with"/>
      <xsl:call-template name="replace-string">
        <xsl:with-param name="text" select="substring-after($text,$replace)"/>
        <xsl:with-param name="replace" select="$replace"/>
        <xsl:with-param name="with" select="$with"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$text"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<!-- MULTI LANGUAGE TEMPLATE FOR INCLUDES -->
<xsl:template match="include">
  <xsl:choose>
    <xsl:when test="@name='C'">
      <xsl:text>&#xA;#include </xsl:text>
      <xsl:value-of select="."/>      
    </xsl:when>
    <xsl:when test="@name='Fortran'">
      <xsl:text>&#xA;  use </xsl:text>
      <xsl:value-of select="."/>
    </xsl:when>
    <xsl:when test="@name='Python'">
      <xsl:text>&#xA;import </xsl:text>
      <xsl:value-of select="."/>
    </xsl:when>
    <xsl:when test="@name='Java'">
      <xsl:text>&#xA;import </xsl:text>
      <xsl:value-of select="."/>      
      <xsl:text>;</xsl:text>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<!-- TEMPLATE TO ADD A SYMBOL AFTER NUMERICS (x.x or exp) IN AN EXPRESSION -->
<xsl:template name="alias">
  <xsl:param name="text"/>
  <xsl:param name="symb"/>
  <xsl:for-each select="str:tokenize($text,' ')">
    <xsl:choose>
      <xsl:when test="contains(.,'.')">
        <xsl:value-of select="."/>
        <xsl:value-of select="$symb"/>
        <xsl:text> </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="."/>
        <xsl:text> </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>

<!-- DOCBOOK TEMPLATES -->

<xsl:template name="docbook">
  <xsl:text>&lt;?xml version="1.0" ?&gt;&#xA;</xsl:text>
  <xsl:text>&lt;section class="topic" id="imas_enum_types__</xsl:text> <xsl:value-of select="$name"/> <xsl:text>"&gt;&#xA;</xsl:text>
  <xsl:text>&lt;title&gt;</xsl:text>
  <xsl:value-of select="$name"/>
  <xsl:text>&lt;/title&gt;&#xA;&#xA;
  </xsl:text>

  <!-- Write comments in the "docbook-screen" -->
  <xsl:text>  &lt;para&gt;&#xA;</xsl:text>
  <xsl:text>    &lt;screen&gt;</xsl:text>
  <xsl:call-template name="replace-string">
    <xsl:with-param name="text" select="concat('&#xA;',header)"/>
    <xsl:with-param name="replace" select="'&#xA;'"/>
    <xsl:with-param name="with" select="'&#xA;'"/>
  </xsl:call-template>
  <xsl:text>    &lt;/screen&gt;&#xA;</xsl:text>
  <xsl:text>  &lt;/para&gt;
  </xsl:text>


  <!-- WARNING!!! TO BE TESTED -->
  <!-- Write where the idensifier can be found in the IDSs -->
  <xsl:if test="/*/ddInstance">
    <xsl:text>&lt;para&gt; This identifier is used in the following places in the ITER IDSs:&#xA;</xsl:text>
    <xsl:for-each select="/*/ddInstance">
      <xsl:text>    &lt;screen&gt;</xsl:text>
      <xsl:value-of select='@xpath'/>
      <xsl:text>&lt;/screen&gt;&#xA;</xsl:text>
    </xsl:for-each>
    <xsl:text>&lt;/para&gt;
    </xsl:text>
  </xsl:if>

  
  <xsl:text>  &#xA;&lt;para&gt; Fortran interface example:&#xA;</xsl:text>
    <xsl:text>   &lt;screen&gt; use </xsl:text>
      <xsl:value-of select='$name'/>
      <xsl:text>, only: get_type_index, get_type_name, get_type_description&lt;/screen&gt;&#xA;</xsl:text>
      <xsl:text>  &lt;/para&gt;&#xA;&#xA;</xsl:text>

  
  <!-- Write a table with all the constants/identifiers -->
  <xsl:choose>
    <xsl:when test="not(int[@unique])">

      <xsl:text>
  &lt;table&gt;
   &lt;tgroup cols="3"&gt;
    &lt;colspec colwidth="45mm"/&gt;
    &lt;colspec colwidth="60mm"/&gt;
    &lt;colspec colwidth="68mm"/&gt;
    &lt;thead&gt;
      &lt;row&gt;
        &lt;entry&gt;Name&lt;/entry&gt;
        &lt;entry&gt;Value&lt;/entry&gt;
        &lt;entry&gt;Description&lt;/entry&gt;
      &lt;/row&gt;
    &lt;/thead&gt;
    &lt;tbody&gt;
    </xsl:text>

    </xsl:when>
    <xsl:otherwise>
  <xsl:text>&lt;table&gt;
   &lt;tgroup cols="3"&gt;. 
  </xsl:text>

        <xsl:text>&lt;colspec colwidth="15mm"/&gt;
    &lt;colspec colwidth="50mm"/&gt;
    &lt;colspec colwidth="108mm"/&gt;
    &lt;thead&gt;
      &lt;row&gt;
        &lt;entry&gt;Flag&lt;/entry&gt;
        &lt;entry&gt;Id&lt;/entry&gt;
        &lt;entry&gt;Description&lt;/entry&gt;
      &lt;/row&gt;
    &lt;/thead&gt;
    &lt;tbody&gt;
    </xsl:text>

    </xsl:otherwise>
  </xsl:choose>

<!--  <xsl:for-each select="*[@name] and not(include)">  -->
  <xsl:for-each select="int | float | boolean | string">
  <xsl:if test="@name">

    <xsl:text>    &lt;row&gt;&#xA;</xsl:text>
    <xsl:text>      &lt;entry&gt; </xsl:text>
    <xsl:value-of select="."/>

    <xsl:text> &lt;/entry&gt;&#xA;</xsl:text>
    <xsl:text>      &lt;entry&gt; &lt;mono&gt;</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:text>&lt;/mono&gt; &lt;/entry&gt;&#xA;</xsl:text>

    <xsl:text>      &lt;entry&gt; </xsl:text>
    <xsl:value-of select="@description"/>
    <xsl:text> &lt;/entry&gt;&#xA;</xsl:text>
    <xsl:text>    &lt;/row&gt;&#xA;</xsl:text>

  </xsl:if>
  </xsl:for-each>
  <xsl:text>
    &lt;/tbody&gt;
   &lt;/tgroup&gt;
  &lt;/table&gt;
  &#xA;</xsl:text>

  <xsl:text>&#xA;&lt;/section&gt;&#xA;&#xA;</xsl:text>

</xsl:template>

</xsl:stylesheet>
