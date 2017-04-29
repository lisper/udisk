#
#
#
# *.L[1-6]
# *.tps    stencil paste top
# *.bps    stencil paste bottom
# *.smb    solder mask bottom
# *.smt    solder mask top
# *.slk	   silk screen top
# *.bsk    silk screen bottom
# *.drd    NC drill data
# *.drl    drill rack
# *.dri    drill info
# *.gpi    gerber info
# 

output:
	mkdir output
	mv *.bps *.bsk *.gpi *.tps *.L[1-6] *.smb *.smt *.slk output
	mv *.drd *.drl *.dri output

zip:
	(cd output; zip freedfm.zip *)

