-- SFD.py - MHS SFD definitions

-- $Header: /xtel/isode/isode/pepy/RCS/SFD.py,v 9.0 1992/06/16 12:25:01 isode Rel $
--
--
-- $Log: SFD.py,v $
-- Revision 9.0  1992/06/16  12:25:01  isode
-- Release 8.0
--
--

--
--				  NOTICE
--
--    Acquisition, use, and distribution of this module and related
--    materials are subject to the restrictions of a license agreement.
--    Consult the Preface in the User's Manual for the full terms of
--    this agreement.
--
--


SFD DEFINITIONS ::=

%{
#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/pepy/RCS/SFD.py,v 9.0 1992/06/16 12:25:01 isode Rel $";
#endif
%}

BEGIN

PRINTER	print

Document	::=  SEQUENCE OF ProtocolElement
	
ProtocolElement	::=  CHOICE {
			textUnit[3] IMPLICIT TextUnit,
			specificLogicalDescriptor[5]
			    IMPLICIT LogicalDescriptor
		     }


-- text units

TextUnit	::=  SEQUENCE {
			contentPortionAttributes ContentPortionAttributes,
			textInformation TextInformation
		     }

ContentPortionAttributes
		::=  SET { --none at present-- }

TextInformation	::=  CHOICE {
			T61String
		     }


-- logical descriptor

LogicalDescriptor
		::=  SEQUENCE { LogicalObjectType, LogicalDescriptorBody }

LogicalObjectType
		::=  INTEGER {
			document (0),
			paragraph (1)
		    }

LogicalDescriptorBody
		::=  SET { 
			-- variable attributes (if object is document) --
			pageHeading[3] IMPLICIT T61String OPTIONAL,
			-- variable attributes (if object is paragraph) --
			layoutDirectives[4] IMPLICIT LayoutDirectives OPTIONAL,
			presentationDirectives[5] IMPLICIT
			    PresentationDirectives OPTIONAL,
			-- default variable attributes for subordinate objects
			--  (if any)
			defaultValueLists[6] IMPLICIT SEQUENCE {
			    DefaultValueList
			} OPTIONAL
		     }

LayoutDirectives::=  SET {
			leftIndentation[0] Offset OPTIONAL,
			bottomBlankLines[3] Offset OPTIONAL
		     }

Offset		::=  CHOICE { [1] IMPLICIT INTEGER}

PresentationDirectives
		::=  SET {
			alignment[0] IMPLICIT Alignment OPTIONAL,
			graphicRendition[1] IMPLICIT GraphicRendition OPTIONAL
		     }

Alignment	::=  INTEGER { leftAligned(0), centered(2), justified(3) }

GraphicRendition::=  SEQUENCE OF GraphicRenditionAspect

GraphicRenditionAspect
		::=  INTEGER --an SGR parameter value; see T.61

DefaultValueList::=  CHOICE {
			paragraphAttributes[1] IMPLICIT ParagraphAttributes
		     }

ParagraphAttributes
		::=  SET {
			layoutDirectives < Attribute OPTIONAL,
			presentationDirectives < Attribute OPTIONAL
		     }

Attribute	::=  CHOICE {
			layoutDirectives[0] IMPLICIT LayoutDirectives,
			presentationDirectives[1] IMPLICIT PresentationDirectives
		     }

END
