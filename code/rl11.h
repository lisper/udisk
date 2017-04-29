/*
RL11 emulation

I/O addresses 774400 - 774407

774400	CSR	read/write
774402	BAR	read/write
774404	DAR	read/write
774406	MPR	read/write

So, address match on 7744xx

RL02:
  2 heads
  40 sectors
  512 cylinders
*/

/* register offsets */
#define	CS	0
#define	BA	2
#define	DA	4
#define	MP	6

/* CS */
#define CS_DRDY		0000001		/* drive ready - r/o */
#define CS_FUNC		0000016		/* function */
#define  CS_FUNC_NOP		0
#define  CS_FUNC_WRITECHK	1
#define  CS_FUNC_GETSTATUS	2
#define  CS_FUNC_SEEK		3
#define  CS_FUNC_READHDR	4
#define  CS_FUNC_WRITE		5
#define  CS_FUNC_READ		6
#define  CS_FUNC_READNOHDR	7

#define CS_BA1617	0000060		/* BA17,BA16 */
#define CS_IE           0000100
#define CS_CRDY		0000200
#define CS_DS		0001400
#define CS_E		0036000

#define CS_E_OPI	(1 << 10)
#define CS_E_DCRC	(2 << 10)
#define CS_E_HCRC	(3 << 10)
#define CS_E_DLT	(4 << 10)
#define CS_E_HNF	(5 << 10)
#define CS_E_NXM	(8 << 10)
#define CS_E_MPE	(9 << 10)

#define CS_DE		0040000		/* drive error */
#define CS_ERR		0100000

#define CS_ANY_ERR	(CS_ERR | (017 << 10))
#define CS_RW		0001776		/* read/write bits */

/* DA */
#define DA_SEEK		0000002
#define DA_DIR		0000004
#define DA_CLR		0000010
#define DA_HS		0000020

/* MP bits for get status */
#define MP_GS_ST	0000007
#define MP_GS_ST_LOAD	0	/* Load cartridge */
#define MP_GS_ST_SPINUP	1	/* Spin-up */
#define MP_GS_ST_BRUSH	2	/* Brush cycle */
#define MP_GS_ST_LOADH	3	/* Load heads */
#define MP_GS_ST_SEEK	4	/* Seeks */
#define MP_GS_ST_LOCK	5	/* lock on */
#define MP_GS_ST_UNLDH	6	/* Unload heads */
#define MP_GS_ST_SPINDN	7	/* Spin-down */

#define MP_GS_BH	0000010		/* brush home */
#define MP_GS_HO	0000020		/* heads out */
#define MP_GS_CO	0000040		/* cover open */
#define MP_GS_HS	0000100		/* head select */
#define MP_GS_DT	0000200		/* drive type */
#define MP_GS_DSE	0000400		/* drive-select error */
#define MP_GS_VC	0001000		/* volume check */
#define MP_GS_WGE	0002000		/* write gate error */
#define MP_GS_SPE	0004000		/* spin error */
#define MP_GS_SKTO	0010000		/* seek time out */
#define MP_GS_WL	0020000		/* write locked */
#define MP_GS_CHE	0040000		/* current head error */
#define MP_GS_WDE	0100000		/* write data error */

#define RL11_BASE	017774400
#define RL11_VECTOR	0160

