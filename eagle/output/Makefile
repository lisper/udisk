
update:
	cp ../udisk.brd .
	cp ../udisk.sch .
	mv ../udisk.L{1,2,3,4} .
	mv ../udisk.tps ../udisk.bps ../udisk.smt ../udisk.smb .
	mv ../udisk.drd ../udisk.dri ../udisk.drl .
	mv ../udisk.bsk ../udisk.gpi ../udisk.slk .

fix:
	../../gfixer/gfixer <udisk.L2 >udisk.L2.fixed && mv udisk.L2 udisk.L2.orig && mv udisk.L2.fixed udisk.L2

view:
	gerbv udisk.L2 udisk.L2.fixed udisk.L3
