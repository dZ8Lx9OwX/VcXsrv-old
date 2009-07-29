@echo off

set OUTFILE=base.ml_s.part

if exist %OUTFILE% del %OUTFILE%

gawk "{ printf """  *		%%s			=	pc+%%s\n""", $1, $2; }" layoutRename.lst >> %OUTFILE%

gawk "{ printf """  *		%%s(%%s)			=	pc+%%s(%%s)\n""", $1, $2, $3, $4;}" variantRename.lst >> %OUTFILE%
