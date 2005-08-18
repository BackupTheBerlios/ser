<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns:db="http://iptel.org/dbschema/mysql"
>

    <xsl:import href="sql.xsl"/>

    <xsl:template name="table.close">
	<xsl:text>)</xsl:text>
	<xsl:if test="db:type">
	    <xsl:text> Type=</xsl:text>
	    <xsl:value-of select="normalize-space(db:type)"/>
	</xsl:if>
	<xsl:text>;&#x0A;&#x0A;</xsl:text>	
    </xsl:template>

    <xsl:template name="column.type">
	<xsl:variable name="type">
	    <xsl:call-template name="get-type"/>
	</xsl:variable>

	<xsl:choose>
	    <xsl:when test="db:type">
		<xsl:value-of select="normalize-space(db:type)"/>
	    </xsl:when>
	    <xsl:when test="$type='char'">
		<xsl:text>TINYINT</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='short'">
		<xsl:text>SMALLINT</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='int'">
		<xsl:text>INT</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='long'">
		<xsl:text>BIGINT</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='datetime'">
		<xsl:text>DATETIME</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='double'">
		<xsl:text>DOUBLE</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='float'">
		<xsl:text>FLOAT</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='string'">
		<xsl:text>VARCHAR</xsl:text>
	    </xsl:when>
	    <xsl:when test="$type='binary'">
		<xsl:text>BLOB</xsl:text>
	    </xsl:when>
	    <xsl:otherwise>
		<xsl:call-template name="type-error"/>
	    </xsl:otherwise>
	</xsl:choose>
    </xsl:template>

    <xsl:template name="column.sign">
	<xsl:variable name="signed">
	    <xsl:call-template name="get-sign"/>
	</xsl:variable>
	
	<xsl:if test="$signed = 0">
	    <xsl:text> UNSIGNED</xsl:text>
	</xsl:if>
    </xsl:template>

    <xsl:template match="index">
	<xsl:variable name="index.name">
	    <xsl:call-template name="get-name"/>
	</xsl:variable>

	<xsl:if test="position()=1">
	    <xsl:text>,&#x0A;</xsl:text>
	</xsl:if>
	<xsl:text>    </xsl:text>
	<xsl:if test="unique">
	    <xsl:text>UNIQUE </xsl:text>
	</xsl:if>
	<xsl:text>KEY </xsl:text>
	<xsl:if test="not($index.name='')">
	    <xsl:value-of select="concat($index.name, ' ')"/>
	</xsl:if>
	<xsl:text>(</xsl:text>
	<xsl:apply-templates select="colref"/>
	<xsl:text>)</xsl:text>
	<xsl:if test="not(position()=last())">
	    <xsl:text>,</xsl:text>
	    <xsl:text>&#x0A;</xsl:text>
	</xsl:if>
    </xsl:template>

</xsl:stylesheet>
