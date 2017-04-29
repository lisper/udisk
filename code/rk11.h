/* */

#define CSR_BIT_GO	0
#define CSR_BIT_IE	6
#define CSR_BIT_DONE	7

#define CS_GO	(1 << CSR_BIT_GO)
#define CS_CRDY (1 << CSR_BIT_DONE)
#define CS_IE	(1 << CSR_BIT_IE)

#define RKCS_CMD_CTLRESET	0
#define RKCS_CMD_WRITE		1
#define RKCS_CMD_READ		2
#define RKCS_CMD_WCHK		3
#define RKCS_CMD_SEEK		4
#define RKCS_CMD_RCHK		5
#define RKCS_CMD_DRVRESET	6
#define RKCS_CMD_WLK		7

#define RKDS_RK05	004000
#define RKDS_RDY 	000200
#define RKDS_RWS 	000100

#define RKDS		000
#define RKER		002
#define RKCS		004
#define RKWC		006
#define RKBA		010
#define RKDA		012

#define RK11_BASE	017777400
#define RK11_VECTOR	0220

