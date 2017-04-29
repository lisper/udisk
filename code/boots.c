#include "main.h"

extern u_short buffer1[];
extern u_short buffer2[];

/*
	Loc.	Cont.	Instruction	Comment
	=======================================
	001000	012700	mov #rlcs,r0	controller address
	001002	174400
	001004	012760	mov #cmd,4(r0)	seek command data
	001006	000013
	001010	000004
	001012	012710	mov #cmd,(r0)	get status
	001014	000004
	001016	105710	tstb (r0)	wait ready
	001020	100376	bpl .-2
	001022	005060	clr 2(r0)	zero bus address
	001024	000002
	001026	005060	clr 4(r0)	zero disk address
	001030	000004
	001032	012760	mov #-256,6(r0)	set the byte count
	001034	177400
	001036	000006
	001040	012710	mov #cmd,(r0)	read block
	001042	000014
	001044	105710	tstb (r0)	wait ready
	001046	100376	bpl .-2
	001050	000000	halt		(or 5007 to auto start)

*/

static u16 boot1[] = {
	0012700,
	0174400,
	0012760,
	0000013,
	0000004,
	0012710,
	0000004,
	0105710,
	0100376,
	0005060,
	0000002,
	0005060,
	0000004,
	0012760,
	0177400,
	0000006,
	0012710,
	0000014,
	0105710,
	0100376,
	0000000,
	0005007
};

void
load_boot1(void)
{
    int i, addr, len;

    printf("write rl boot loader\n");
    addr = 01000;
    len = sizeof(boot1) / 2;

    printf("last %o\n", addr+len*2);

    unibus_dma_buffer(1, addr, boot1, len);
}

/*
	0012706, 0002000,		MOV #2000, SP 
	0012700, 0000000,		MOV #UNIT, R0 
	0010003,			MOV R0, R3 
	0000303,			SWAB R3 
	0012701, 0174400,		MOV #RLCS, R1 		; csr 
	0012761, 0000013, 0000004,	MOV #13, 4(R1)		; clr err 
	0052703, 0000004,		BIS #4, R3		; unit+gstat 
	0010311,			MOV R3, (R1)		; issue cmd 
	0105711,			TSTB (R1)		; wait 
	0100376,			BPL .-2 
	0105003,			CLRB R3 
	0052703, 0000010,		BIS #10, R3		; unit+rdhdr 
	0010311,			MOV R3, (R1)		; issue cmd 
	0105711,			TSTB (R1)		; wait 
	0100376,			BPL .-2 
	0016102, 0000006,		MOV 6(R1), R2		; get hdr 
	0042702, 0000077,		BIC #77, R2		; clr sector 
	0005202,			INC R2			; magic bit 
	0010261, 0000004,		MOV R2, 4(R1)		; seek to 0 
	0105003,			CLRB R3 
	0052703, 0000006,		BIS #6, R3		; unit+seek 
	0010311,			MOV R3, (R1)		; issue cmd 
	0105711,			TSTB (R1)		; wait 
	0100376,			BPL .-2 
	0005061, 0000002,		CLR 2(R1)		; clr ba 
	0005061, 0000004,		CLR 4(R1)		; clr da 
	0012761, 0177000, 0000006,	MOV #-512., 6(R1)	; set wc 
	0105003,			CLRB R3 
	0052703, 0000014,		BIS #14, R3		; unit+read 
	0010311,			MOV R3, (R1)		; issue cmd 
	0105711,			TSTB (R1)		; wait 
	0100376,			BPL .-2 
	0042711, 0000377,		BIC #377, (R1) 
	0005002,			CLR R2 
	0005003,			CLR R3 
	0005004,			CLR R4 
	0012705, 0062154,		MOV "DL, R5 
	0005007				CLR PC 
*/

u16 boot2[] = {
#define BOOT2_START 02000
    0012706, BOOT2_START,	/* MOV #boot_start, SP */
    0012700, 0000000,		/* MOV #unit, R0 */
    0010003,			/* MOV R0, R3 */
    0000303,			/* SWAB R3 */
    0012701, 0174400,		/* MOV #RLCS, R1    ; csr */
    0012761, 0000013, 0000004,	/* MOV #13, 4(R1)   ; clr err */
    0052703, 0000004,		/* BIS #4, R3	    ; unit+gstat */
    0010311,			/* MOV R3, (R1)	    ; issue cmd */
    0105711,			/* TSTB (R1)	    ; wait */
    0100376,			/* BPL .-2 */
    0105003,			/* CLRB R3 */
    0052703, 0000010,		/* BIS #10, R3	    ; unit+rdhdr */
    0010311,			/* MOV R3, (R1)	    ; issue cmd */
    0105711,			/* TSTB (R1)	    ; wait */
    0100376,			/* BPL .-2 */
    0016102, 0000006,		/* MOV 6(R1), R2    ; get hdr */
    0042702, 0000077,		/* BIC #77, R2	    ; clr sector */
    0005202,			/* INC R2	    ; magic bit */
    0010261, 0000004,		/* MOV R2, 4(R1)    ; seek to 0 */
    0105003,			/* CLRB R3 */
    0052703, 0000006,		/* BIS #6, R3	    ; unit+seek */
    0010311,			/* MOV R3, (R1)	    ; issue cmd */
    0105711,			/* TSTB (R1)	    ; wait */
    0100376,			/* BPL .-2 */
    0005061, 0000002,		/* CLR 2(R1)	    ; clr ba */
    0005061, 0000004,		/* CLR 4(R1)	    ; clr da */
    0012761, 0177000, 0000006,	/* MOV #-512., 6(R1)    ; set wc */
    0105003,			/* CLRB R3 */
    0052703, 0000014,		/* BIS #14, R3	    ; unit+read */
    0010311,			/* MOV R3, (R1)	    ; issue cmd */
    0105711,			/* TSTB (R1)	    ; wait */
    0100376,			/* BPL .-2 */
    0042711, 0000377,		/* BIC #377, (R1) */
    0005002,			/* CLR R2 */
    0005003,			/* CLR R3 */
    0012704, BOOT2_START+016,	/* MOV #START+20, R4 */
    0005005,			/* CLR R5 */
    0005007			/* CLR PC */
};

void
load_boot2(void)
{
    u_short addr;
    int len;

    printf("write rl boot loader\n");
    addr = 02000;
    len = sizeof(boot2) / 2;

#define BOOT_UNIT       (BOOT_START + 010)              /* unit number */
#define BOOT_CSR        (BOOT_START + 016)              /* CSR */

    printf("last %o\n", addr+len);
    printf("len %d words\n", len);

    unibus_dma_buffer(1, addr, boot2, len);
}

u16 boot3[] = {
#define BOOT3_START 02000
    0012706, BOOT3_START,          /* MOV #boot_start, SP */
    0012700, 0000000,              /* MOV #unit, R0 ; unit number */
    0010003,                       /* MOV R0, R3 */
    0000303,                       /* SWAB R3 */
    0006303,                       /* ASL R3 */
    0006303,                       /* ASL R3 */
    0006303,                       /* ASL R3 */
    0006303,                       /* ASL R3 */
    0006303,                       /* ASL R3 */
    0012701, 0177412,              /* MOV #RKDA, R1        ; csr */
    0010311,                       /* MOV R3, (R1)         ; load da */
    0005041,                       /* CLR -(R1)            ; clear ba */
    0012741, 0177000,              /* MOV #-256.*2, -(R1)  ; load wc */
    0012741, 0000005,              /* MOV #READ+GO, -(R1)  ; read & go */
    0005002,                       /* CLR R2 */
    0005003,                       /* CLR R3 */
    0012704, BOOT3_START+020,      /* MOV #START+20, R4 */
    0005005,                       /* CLR R5 */
    0105711,                       /* TSTB (R1) */
    0100376,                       /* BPL .-2 */
    0105011,                       /* CLRB (R1) */
    0005007                        /* CLR PC */
};

void
load_boot3(void)
{
    u_short addr;
    int len;

    printf("write rk boot loader\n");
    addr = 02000;
    len = sizeof(boot3) / 2;
#undef BOOT_UNIT
#undef BOOT_CSR
#define BOOT_UNIT       (BOOT_START + 010)              /* unit number */
#define BOOT_CSR        (BOOT_START + 032)              /* CSR */

    printf("last %o\n", addr+len);
    printf("len %d words\n", len);

    unibus_dma_buffer(1, addr, boot3, len);
}

void
load_int(void)
{
    int i, addr;

    printf("write int test\n");
    addr = 01000;

    /* make sure 160 is 0 (halt) */
    buffer1[0] = 0;
    buffer1[1] = 0;
    unibus_dma_buffer(1, 0160, buffer1, 2);

    i = 0;
    buffer1[i++] = 0012706; /* MOV #2000, SP */
    buffer1[i++] = 0002000;

    buffer1[i++] = 0012767; /* move $30340,PS */
    buffer1[i++] = 0030340;
    buffer1[i++] = 0177776;

    buffer1[i++] = 0000230;  /* spl 0 */

    buffer1[i++] = 0000240; /* nop */
    buffer1[i++] = 0000776; /* jmp .-2 */

    printf("last %o\n", addr+i*2);

    unibus_dma_buffer(1, addr, buffer1, i);
}

void
load_int_rti(void)
{
    int i, addr;

    printf("write int test\n");
    addr = 01000;

    /* code at vector 160 is inc r2; rti */
    buffer1[0] = 0000164;
    buffer1[1] = 0000340;
    buffer1[2] = 0005204;	/* INC R4 */
    buffer1[3] = 0010460;
    buffer1[4] = 0000000;	/* MOV R4, 0(R0) */
    buffer1[5] = 2;
    unibus_dma_buffer(1, 0160, buffer1, 6);

    printf("rti @ 160\n");

    i = 0;
    buffer1[i++] = 0012700; /* mov #0,r0 */
    buffer1[i++] = 0000000;

    buffer1[i++] = 0005004; /* clr r4 */

    buffer1[i++] = 0005060; /* clr 0(r0) */
    buffer1[i++] = 0000000;

    buffer1[i++] = 0012706; /* MOV #2000, SP */
    buffer1[i++] = 0002000;

    buffer1[i++] = 0012767; /* move $30340,PS */
    buffer1[i++] = 0030340;
    buffer1[i++] = 0177776;

    buffer1[i++] = 0000230;  /* spl 0 */

    buffer1[i++] = 0000240; /* nop */
    buffer1[i++] = 0000776; /* jmp .-2 */

    printf("last %o\n", addr+i*2);

    unibus_dma_buffer(1, addr, buffer1, i);
}

void
load_poll(int arg)
{
    int i, addr;
    int which;

    printf("write poll test\n");
    addr = 01000;

    which = 5;
    if (arg >= 0)
        which = arg;

    i = 0;

    printf("loading test %d\n", which);
    switch (which) {
    case 0:
        buffer1[i++] = 0012700; /* move #rlcs,r0 */
        buffer1[i++] = 0174400;
        buffer1[i++] = 0240;	/* nop */
        buffer1[i++] = 0240;	/* nop */
        buffer1[i++] = 0105710; /* tstb (r0) */
        buffer1[i++] = 0000774; /* jmp .-4 */
        break;

    case 1:
        buffer1[i++] = 0x15c0; /* 012700 mov #rlcs, r0 */
        buffer1[i++] = 0xf900; /* 174400 */
        buffer1[i++] = 0x8bc8; /* 105710 tstb	(r0) */
        buffer1[i++] = 0x01fe; /* 000167 jmp	.-4 <loop> */
        break;

    case 2:
        buffer1[i++] = 0012700; /* move #rlcs,r0 */
        buffer1[i++] = 0174400;

        buffer1[i++] = 0012760; /* mov #-256,6(r0)	set the byte count */
        buffer1[i++] = 0177400;
        buffer1[i++] = 0000006;

        buffer1[i++] = 0012710; /* mov $14, (r0) */
        buffer1[i++] = 0000014;
//buffer1[i++] = 0240;
        buffer1[i++] = 0105710; /* tstb (r0) */
        buffer1[i++] = 0100376; /* bpl .-2 */
        buffer1[i++] = 0000000; /* halt */
        break;

    case 3:
        buffer1[i++] = 0x15c0; /* 012700 mov #rlcs,ro */
        buffer1[i++] = 0xf900; /* 174400 */
        buffer1[i++] = 0x15c8; /* 012710 mov $016, (r0) */
        buffer1[i++] = 0x000e; /* 000016 */
        buffer1[i++] = 0x8bc8; /* 105710 tstb	(r0) */
        buffer1[i++] = 0x0077; /* 000167 jmp	.-4 <loop> */
        buffer1[i++] = 0xfff2; /* 177762 */
        break;

    case 4:
        buffer1[i++] = 0012700; /* mov #rlcs,ro */
        buffer1[i++] = 0174400;
        buffer1[i++] = 0011001; /* mov (r0), r1 */
        buffer1[i++] = 0016001; /* mov 2(r0), r1 */
        buffer1[i++] = 0000002;
        buffer1[i++] = 0016001; /* mov 4(r0), r1 */
        buffer1[i++] = 0000004;
        buffer1[i++] = 0016001; /* mov 6(r0), r1 */
        buffer1[i++] = 0000006;
        buffer1[i++] = 0000167; /* jmp .-4 */
        buffer1[i++] = 0177752;
        break;

    case 5:
        buffer1[i++] = 0012700; /* move #rlcs,r0 */
        buffer1[i++] = 0174400;
        buffer1[i++] = 0011001; /* loop: mov (r0), r1  */
        buffer1[i++] = 0012702; /* mov #01230,r2 */
        buffer1[i++] = 0001230;
        buffer1[i++] = 0020102; /* cmp r1, r2 */
        buffer1[i++] = 0001773; /* beq loop */
        buffer1[i++] = 0000000; /* halt */
        buffer1[i++] = 0000771; /* br loop */
        break;

    case 6:
        buffer1[i++] = 0012700; /* move #rlcs,r0 */
        buffer1[i++] = 0174400;
        buffer1[i++] = 0105710;	/* tstb (r0) */
        buffer1[i++] = 0000776; /* br .-2 */
        break;
    }

    printf("last %o\n", addr+i*2);

    unibus_dma_buffer(1, addr, buffer1, i);

    addr = 02000;
    buffer1[0] = 0240;
    buffer1[1] = 0776;
    unibus_dma_buffer(1, addr, buffer1, 2);
}

u16 regs1[] = {
	/* regs1.mac */
	0012700, // mov #RLDA,r0
	0174404,
	0012702, // mov #1,r2
	0000001,
	0011001,
	0010210,
	0005210,
	0005210,
	0005210,
	0011003,
	0022703,
	0000004,
	0001401,
	0000000,
	0011001, // mov (r0), r1
	0005010, // clr (r0)
	0052710,
	0000010,
	0052710,
	0000020,
	0052710,
	0000001,
	0011003,
	0022703,
	0000031,
	0001401,
	0000000,
	0012702,
	0000777,
	0011001,
	0010210,
	0042710, // bic #10,(r0)	;767
	0000010,
	0042710, // bic #20,(r0)	;747
	0000020,
	0042710, // bic #01,(r0)	;746
	0000001,
	0011003,
	0022703,
	0000746,
	0001401,
	0000000,
	0012700,
	0174400,
	0105710,
	0105710,
	0105710,
	0000720
};

void
load_regs(void)
{
    int i, addr, len;

    printf("loading write reg test\n");
    addr = 01000;
    len = sizeof(boot1) / 2;

    printf("last %o\n", addr+len*2);

    unibus_dma_buffer(1, addr, regs1, len);
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
