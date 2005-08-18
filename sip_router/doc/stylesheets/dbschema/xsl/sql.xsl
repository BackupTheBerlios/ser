<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
>

    <xsl:import href="common.xsl"/>

    <xsl:template match="/">
	<xsl:variable name="createfile" select="concat($prefix, concat('/', 'create.sql'))"/>
	<xsl:document href="{$createfile}" method="text" indent="no" omit-xml-declaration="yes">
	    <xsl:apply-templates select="/database[1]"/>
	</xsl:document>

	<xsl:variable name="dropfile" select="concat($prefix, concat('/', 'drop.sql'))"/>
	<xsl:document href="{$dropfile}" method="text" indent="no" omit-xml-declaration="yes">
	    <xsl:apply-templates mode="drop" select="/database[1]"/>
	</xsl:document>
    </xsl:template>

<!-- ################ DATABASE ################# -->

    <xsl:template match="database">
	<xsl:variable name="database.name">
	    <xsl:call-template name="get-name"/>
	</xsl:variable>

	<xsl:text>CREATE DATABASE </xsl:text>
	<xsl:value-of select="$database.name"/>
	<xsl:text>;&#x0A;</xsl:text>

	<xsl:text>USE </xsl:text>
	<xsl:value-of select="$database.name"/>
	<xsl:text>;&#x0A;&#x0A;</xsl:text>

	<xsl:apply-imports/>
    </xsl:template>

    <xsl:template match="database" mode="drop">
	<xsl:variable name="database.name">
	    <xsl:call-template name="get-name"/>
	</xsl:variable>

	<xsl:text>USE </xsl:text>
	<xsl:value-of select="$database.name"/>
	<xsl:text>;&#x0A;&#x0A;</xsl:text>
	<xsl:apply-templates mode="drop" select="table"/>
	<xsl:text>&#x0A;</xsl:text>
	<xsl:text>DROP DATABASE </xsl:text>
	<xsl:value-of select="$database.name"/>
	<xsl:text>;&#x0A;</xsl:text>
    </xsl:template>
    
<!-- ################ /DATABASE ################# -->

    
<!-- ################ TABLE  ################# -->
    
    <xsl:template match="table">
	<xsl:variable name="table.name">
	    <xsl:call-template name="get-name"/>
	</xsl:variable>

	<xsl:text>CREATE TABLE </xsl:text>
	<xsl:value-of select="$table.name"/>
	<xsl:text> (&#x0A;</xsl:text>

	<!-- Process all columns -->
	<xsl:apply-templates select="column"/>

	<!-- Process all indexes -->
	<xsl:apply-templates select="index"/>

	<xsl:text>&#x0A;</xsl:text>

	<xsl:call-template name="table.close"/>

	<!-- Process initial rows of data -->
	<xsl:apply-templates select="row"/>
    </xsl:template>

    <xsl:template match="table" mode="drop">
	<xsl:text>DROP TABLE </xsl:text>
	<xsl:call-template name="get-name"/>
	<xsl:text>;&#x0A;</xsl:text>
    </xsl:template>

    <xsl:template name="table.close">
	<xsl:text>);&#x0A;&#x0A;</xsl:text>
    </xsl:template>

<!-- ################ /TABLE ################  -->

<!-- ################ COLUMN ################  -->

    <xsl:template match="column">
	<xsl:text>    </xsl:text>
	<xsl:call-template name="get-name"/>
	<xsl:text> </xsl:text>

	<xsl:call-template name="column.type"/>

	<xsl:call-template name="column.size"/>

	<xsl:call-template name="column.sign"/>

	<xsl:variable name="null">
	    <xsl:call-template name="get-null"/>
	</xsl:variable>
	<xsl:if test="$null=0">
	    <xsl:text> NOT NULL</xsl:text>
	</xsl:if>

	<xsl:choose>
	    <xsl:when test="db:default">
		<xsl:text> DEFAULT </xsl:text>
		<xsl:choose>
		    <xsl:when test="db:default/null">
			<xsl:text>NULL</xsl:text>
		    </xsl:when>
		    <xsl:otherwise>
			<xsl:text>'</xsl:text>
			<xsl:value-of select="db:default"/>
			<xsl:text>'</xsl:text>
		    </xsl:otherwise>
		</xsl:choose>
	    </xsl:when>
	    <xsl:when test="default">
		<xsl:text> DEFAULT </xsl:text>
		<xsl:choose>
		    <xsl:when test="default/null">
			<xsl:text>NULL</xsl:text>
		    </xsl:when>
		    <xsl:otherwise>
			<xsl:text>'</xsl:text>
			<xsl:value-of select="default"/>
			<xsl:text>'</xsl:text>
		    </xsl:otherwise>
		</xsl:choose>
	    </xsl:when>
	</xsl:choose>

	<xsl:if test="not(position()=last())">
	    <xsl:text>,</xsl:text>
	    <xsl:text>&#x0A;</xsl:text>
	</xsl:if>
    </xsl:template>

    <xsl:template name="column.type">
	<xsl:call-template name="get-type"/>
    </xsl:template>

    <xsl:template name="column.size">
	<xsl:variable name="size">
	    <xsl:call-template name="get-size"/>
	</xsl:variable>

	<xsl:if test="not($size='')">
	    <xsl:text>(</xsl:text>
	    <xsl:value-of select="$size"/>
	    <xsl:text>)</xsl:text>
	</xsl:if>
    </xsl:template>

    <xsl:template name="column.sign"/>

<!-- ################ /COLUMN ################  -->

<!-- ################ INDEX ################  -->

    <xsl:template match="index">
	<!-- Translate unique indexes into SQL92 unique constraints -->
	<xsl:if test="unique">
	    <xsl:if test="position()=1">
		<xsl:text>,&#x0A;</xsl:text>
	    </xsl:if>
	    <xsl:text>    </xsl:text>

	    <xsl:call-template name="get-name"/>
	    <xsl:text> UNIQUE (</xsl:text>

	    <xsl:apply-templates match="colref"/>

	    <xsl:text>)</xsl:text>

	    <xsl:if test="not(position()=last())">
		<xsl:text>,</xsl:text>
		<xsl:text>&#x0A;</xsl:text>
	    </xsl:if>
	</xsl:if>
    </xsl:template>

<!-- ################ /INDEX ################  -->

<!-- ################ COLREF ################  -->

    <xsl:template match="colref">
	<xsl:variable name="columns" select="key('column_id', @linkend)"/>
	<xsl:variable name="column" select="$columns[1]"/>
	<xsl:choose>
	    <xsl:when test="count($column) = 0">
		<xsl:message terminate="yes">
		    <xsl:text>ERROR: Column with id '</xsl:text>
		    <xsl:value-of select="@linkend"/>
		    <xsl:text>' does not exist.</xsl:text>
		</xsl:message>
	    </xsl:when>
	    <xsl:otherwise>
		<xsl:call-template name="get-name">
		    <xsl:with-param name="select" select="$column"/>
		</xsl:call-template>
	    </xsl:otherwise>
	</xsl:choose>
	<xsl:if test="not(position()=last())">
	    <xsl:text>, </xsl:text>
	</xsl:if>
    </xsl:template>

<!-- ################ /COLREF ################  -->

<!-- ################ ROW ################  -->

    <xsl:template match="row">
	<xsl:text>INSERT INTO </xsl:text>
	<xsl:call-template name="get-name">
	    <xsl:with-param name="select" select="parent::table"/>
	</xsl:call-template>
	<xsl:text> (</xsl:text>
	<xsl:apply-templates select="value" mode="colname"/>
	<xsl:text>) VALUES (</xsl:text>
	<xsl:apply-imports/>
	<xsl:text>);&#x0A;</xsl:text>
	<xsl:if test="position()=last()">
	    <xsl:text>&#x0A;</xsl:text>	    
	</xsl:if>
    </xsl:template>

<!-- ################ /ROW ################  -->

<!-- ################ VALUE ################  -->

    <xsl:template match="value">
	<xsl:choose>
	    <xsl:when test="null">
		<xsl:text>NULL</xsl:text>
	    </xsl:when>
	    <xsl:otherwise>
		<xsl:text>'</xsl:text>
		<xsl:value-of select="text()"/>
		<xsl:text>'</xsl:text>
	    </xsl:otherwise>
	</xsl:choose>
	<xsl:if test="not(position()=last())">
	    <xsl:text>, </xsl:text>
	</xsl:if>
    </xsl:template>

    <xsl:template match="value" mode="colname">
	<xsl:variable name="columns" select="key('column_id', @col)"/>
	<xsl:variable name="column" select="$columns[1]"/>
	<xsl:choose>
	    <xsl:when test="count($column) = 0">
		<xsl:message terminate="yes">
		    <xsl:text>ERROR: Column with id '</xsl:text>
		    <xsl:value-of select="@col"/>
		    <xsl:text>' does not exist.</xsl:text>
		</xsl:message>
	    </xsl:when>
	    <xsl:otherwise>
		<xsl:call-template name="get-name">
		    <xsl:with-param name="select" select="$column"/>
		</xsl:call-template>
	    </xsl:otherwise>
	</xsl:choose>
	<xsl:if test="not(position()=last())">
	    <xsl:text>, </xsl:text>
	</xsl:if>
    </xsl:template>

<!-- ################ /VALUE ################  -->

</xsl:stylesheet>