/*
 * usb.c
 * $Id: usb.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"
#include "board.h"
#include "usb.h"
#include "flash.h"

/*const */char devDescriptor[] = {
	/* Device descriptor */
	0x12,   // bLength
	0x01,   // bDescriptorType
	0x10,   // bcdUSBL
	0x01,   //
	0x02,   // bDeviceClass:    CDC class code
	0x00,   // bDeviceSubclass: CDC class sub code
	0x00,   // bDeviceProtocol: CDC Device protocol
	0x08,   // bMaxPacketSize0
	0xeb,   // idVendorL
	0x03,   //
	0x25,   // idProductL
	0x61,   //
	0x10,   // bcdDeviceL
	0x01,   //
	0x00,   // iManufacturer    // 0x01
	0x00,   // iProduct
	0x00,   // SerialNumber
	0x01    // bNumConfigs
};

/*const */char cfgDescriptor[] = {
	/* ============== CONFIGURATION 1 =========== */
	/* Configuration 1 descriptor */
	0x09,   // CbLength
	0x02,   // CbDescriptorType
	0x43,   // CwTotalLength 2 EP + Control
	0x00,
	0x02,   // CbNumInterfaces
	0x01,   // CbConfigurationValue
	0x00,   // CiConfiguration
	0xC0,   // CbmAttributes 0xA0
	0x00,   // CMaxPower

	/* Communication Class Interface Descriptor Requirement */
	0x09, // bLength
	0x04, // bDescriptorType
	0x00, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints
	0x02, // bInterfaceClass
	0x02, // bInterfaceSubclass
	0x00, // bInterfaceProtocol
	0x00, // iInterface

	/* Header Functional Descriptor */
	0x05, // bFunction Length
	0x24, // bDescriptor type: CS_INTERFACE
	0x00, // bDescriptor subtype: Header Func Desc
	0x10, // bcdCDC:1.1
	0x01,

	/* ACM Functional Descriptor */
	0x04, // bFunctionLength
	0x24, // bDescriptor Type: CS_INTERFACE
	0x02, // bDescriptor Subtype: ACM Func Desc
	0x00, // bmCapabilities

	/* Union Functional Descriptor */
	0x05, // bFunctionLength
	0x24, // bDescriptorType: CS_INTERFACE
	0x06, // bDescriptor Subtype: Union Func Desc
	0x00, // bMasterInterface: Communication Class Interface
	0x01, // bSlaveInterface0: Data Class Interface

	/* Call Management Functional Descriptor */
	0x05, // bFunctionLength
	0x24, // bDescriptor Type: CS_INTERFACE
	0x01, // bDescriptor Subtype: Call Management Func Desc
	0x00, // bmCapabilities: D1 + D0
	0x01, // bDataInterface: Data Class Interface 1

	/* Endpoint 1 descriptor */
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x83,   // bEndpointAddress, Endpoint 03 - IN
	0x03,   // bmAttributes      INT
	0x08,   // wMaxPacketSize
	0x00,
	0xFF,   // bInterval

	/* Data Class Interface Descriptor Requirement */
	0x09, // bLength
	0x04, // bDescriptorType
	0x01, // bInterfaceNumber
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints
	0x0A, // bInterfaceClass
	0x00, // bInterfaceSubclass
	0x00, // bInterfaceProtocol
	0x00, // iInterface

	/* First alternate setting */
	/* Endpoint 1 descriptor */
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x01,   // bEndpointAddress, Endpoint 01 - OUT
	0x02,   // bmAttributes      BULK
	AT91C_EP_OUT_SIZE,   // wMaxPacketSize
	0x00,
	0x00,   // bInterval

	/* Endpoint 2 descriptor */
	0x07,   // bLength
	0x05,   // bDescriptorType
	0x82,   // bEndpointAddress, Endpoint 02 - IN
	0x02,   // bmAttributes      BULK
	AT91C_EP_IN_SIZE,   // wMaxPacketSize
	0x00,
	0x00    // bInterval
};


AT91S_CDC_LINE_CODING line = {
	115200, // baudrate
	0,      // 1 Stop Bit
	0,      // None Parity
	8};     // 8 Data bits


AT91PS_UDP pUdp;
unsigned char currentConfiguration;
unsigned char currentConnection;
unsigned int  currentRcvBank;

void usb_enumerate(void);

void
usb_setup(void)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    /* set the PLL USB divider */
    AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1 ;

    // Specific Chip USB Initialisation
    // Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDP);

    /* set up pullup pio */
    pPio->PIO_OWER = P_USB_PUP;
    pPio->PIO_PER = P_USB_PUP;
    pPio->PIO_OER = P_USB_PUP;
}

void usb_open(void)
{
    AT91PS_PIO pPio = AT91C_BASE_PIOA;

    pUdp = AT91C_BASE_UDP;
    currentConfiguration = 0;
    currentConnection    = 0;
    currentRcvBank       = AT91C_UDP_RX_DATA_BK0;

    /* clear to enable the pull up resistor */
    pPio->PIO_CODR = P_USB_PUP;
}


int usb_isconfigured()
{
    AT91_REG isr = pUdp->UDP_ISR;

    if (isr & AT91C_UDP_ENDBUSRES) {
        pUdp->UDP_ICR = AT91C_UDP_ENDBUSRES;

        // reset all endpoints
        pUdp->UDP_RSTEP  = (unsigned int)-1;
        pUdp->UDP_RSTEP  = 0;

        // Enable the function
        pUdp->UDP_FADDR = AT91C_UDP_FEN;

        // Configure endpoint 0
        pUdp->UDP_CSR[0] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL);

        // Meins
        currentConfiguration = 0;
    } else
        if (isr & AT91C_UDP_EPINT0) {
            pUdp->UDP_ICR = AT91C_UDP_EPINT0;
            usb_enumerate();
        }

    return currentConfiguration;
}


int usb_dataready()
{
    if ( (pUdp->UDP_CSR[AT91C_EP_OUT] >> 16) )
        return 1;

    return 0;
}

uint usb_read(char *pData, uint length)
{
    uint packetSize, nbBytesRcv = 0, currentReceiveBank = currentRcvBank;

    while (length) {
        if ( !usb_isconfigured() )
            break;

        if ( pUdp->UDP_CSR[AT91C_EP_OUT] & currentReceiveBank ) {
            packetSize = MIN(pUdp->UDP_CSR[AT91C_EP_OUT] >> 16, length);

            length -= packetSize;

            if (packetSize < AT91C_EP_OUT_SIZE)
                length = 0;

            while(packetSize--)
                pData[nbBytesRcv++] = pUdp->UDP_FDR[AT91C_EP_OUT];

            pUdp->UDP_CSR[AT91C_EP_OUT] &= ~(currentReceiveBank);

            if (currentReceiveBank == AT91C_UDP_RX_DATA_BK0)
                currentReceiveBank = AT91C_UDP_RX_DATA_BK1;
            else
                currentReceiveBank = AT91C_UDP_RX_DATA_BK0;
        }
    }

    currentRcvBank = currentReceiveBank;
    return nbBytesRcv;
}

uint usb_write(char *pData, uint length)
{
    uint cpt = 0;

    // Send the first packet
    cpt = MIN(length, AT91C_EP_IN_SIZE);
    length -= cpt;

    while (cpt--) pUdp->UDP_FDR[AT91C_EP_IN] = *pData++;

    pUdp->UDP_CSR[AT91C_EP_IN] |= AT91C_UDP_TXPKTRDY;

    while (length) {
        // Fill the second bank
        cpt = MIN(length, AT91C_EP_IN_SIZE);
        length -= cpt;

        while (cpt--) pUdp->UDP_FDR[AT91C_EP_IN] = *pData++;

        // Wait for the the first bank to be sent
        while ( !(pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP) )
            if ( !usb_isconfigured() ) return length;

        pUdp->UDP_CSR[AT91C_EP_IN] &= ~(AT91C_UDP_TXCOMP);
        while (pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP);

        pUdp->UDP_CSR[AT91C_EP_IN] |= AT91C_UDP_TXPKTRDY;
    }

    // Wait for the end of transfer
    while ( !(pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP) )
        if ( !usb_isconfigured() ) return length;

    pUdp->UDP_CSR[AT91C_EP_IN] &= ~(AT91C_UDP_TXCOMP);

    while (pUdp->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP);

    return length;
}

void usb_senddata(char *pData, uint length)
{
    uint cpt = 0;
    AT91_REG csr;

    do {
        cpt = MIN(length, 8);
        length -= cpt;

        while (cpt--)
            pUdp->UDP_FDR[0] = *pData++;

        if (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP) {
            pUdp->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP);
            while (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP);
        }

        pUdp->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY;
        do {
            csr = pUdp->UDP_CSR[0];

            // Data IN stage has been stopped by a status OUT
            if (csr & AT91C_UDP_RX_DATA_BK0) {
                pUdp->UDP_CSR[0] &= ~(AT91C_UDP_RX_DATA_BK0);
                return;
            }
        } while ( !(csr & AT91C_UDP_TXCOMP) );

    } while (length);

    if (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP) {
        pUdp->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP);
        while (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP);
    }
}

void usb_sendzlp()
{
    pUdp->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY;
    while ( !(pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP) );

    pUdp->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP);
    while (pUdp->UDP_CSR[0] & AT91C_UDP_TXCOMP);
}

void usb_sendstall()
{
    pUdp->UDP_CSR[0] |= AT91C_UDP_FORCESTALL;
    while ( !(pUdp->UDP_CSR[0] & AT91C_UDP_ISOERROR) );

    pUdp->UDP_CSR[0] &= ~(AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR);
    while (pUdp->UDP_CSR[0] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR));
}

void usb_enumerate(void)
{
    uchar bmRequestType, bRequest;
    ushort wValue, wIndex, wLength, wStatus;

    if ( !(pUdp->UDP_CSR[0] & AT91C_UDP_RXSETUP) )
        return;

    bmRequestType = pUdp->UDP_FDR[0];
    bRequest      = pUdp->UDP_FDR[0];
    wValue        = (pUdp->UDP_FDR[0] & 0xFF);
    wValue       |= (pUdp->UDP_FDR[0] << 8);
    wIndex        = (pUdp->UDP_FDR[0] & 0xFF);
    wIndex       |= (pUdp->UDP_FDR[0] << 8);
    wLength       = (pUdp->UDP_FDR[0] & 0xFF);
    wLength      |= (pUdp->UDP_FDR[0] << 8);

    if (bmRequestType & 0x80) {
        pUdp->UDP_CSR[0] |= AT91C_UDP_DIR;
        while ( !(pUdp->UDP_CSR[0] & AT91C_UDP_DIR) );
    }

    pUdp->UDP_CSR[0] &= ~AT91C_UDP_RXSETUP;
    while ( (pUdp->UDP_CSR[0]  & AT91C_UDP_RXSETUP)  );

    if (0) printf("usb_enumerate() %x\n", (bRequest << 8) | bmRequestType);

    // Handle supported standard device request Cf Table 9-3 in USB specification Rev 1.1
    switch ((bRequest << 8) | bmRequestType) {
    case STD_GET_DESCRIPTOR:
        if (wValue == 0x100)       // Return Device Descriptor
            usb_senddata(devDescriptor, MIN(sizeof(devDescriptor), wLength));
        else if (wValue == 0x200)  // Return Configuration Descriptor
            usb_senddata(cfgDescriptor, MIN(sizeof(cfgDescriptor), wLength));
        else
            usb_sendstall();
        break;

    case STD_SET_ADDRESS:
        usb_sendzlp();
        pUdp->UDP_FADDR = (AT91C_UDP_FEN | wValue);
        pUdp->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_FADDEN : 0;
        break;

    case STD_SET_CONFIGURATION:
        currentConfiguration = wValue;
        usb_sendzlp();
        pUdp->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_CONFG : AT91C_UDP_FADDEN;
        pUdp->UDP_CSR[1] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT) : 0;
        pUdp->UDP_CSR[2] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN)  : 0;
        pUdp->UDP_CSR[3] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_IN)   : 0;
        break;

    case STD_GET_CONFIGURATION:
        usb_senddata((char *) &(currentConfiguration), sizeof(currentConfiguration));
        break;

    case STD_GET_STATUS_ZERO:
        wStatus = 0;
        usb_senddata((char *) &wStatus, sizeof(wStatus));
        break;

    case STD_GET_STATUS_INTERFACE:
        wStatus = 0;
        usb_senddata((char *) &wStatus, sizeof(wStatus));
        break;

    case STD_GET_STATUS_ENDPOINT:
        wStatus = 0;
        wIndex &= 0x0F;
        if ((pUdp->UDP_GLBSTATE & AT91C_UDP_CONFG) && (wIndex <= 3)) {
            wStatus = (pUdp->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
            usb_senddata((char *) &wStatus, sizeof(wStatus));
        }
        else if ((pUdp->UDP_GLBSTATE & AT91C_UDP_FADDEN) && (wIndex == 0)) {
            wStatus = (pUdp->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
            usb_senddata((char *) &wStatus, sizeof(wStatus));
        }
        else
            usb_sendstall();
        break;

    case STD_SET_FEATURE_ZERO:
        usb_sendstall();
        break;

    case STD_SET_FEATURE_INTERFACE:
        usb_sendzlp();
        break;

    case STD_SET_FEATURE_ENDPOINT:
        wIndex &= 0x0F;
        if ((wValue == 0) && wIndex && (wIndex <= 3)) {
            pUdp->UDP_CSR[wIndex] = 0;
            usb_sendzlp();
        }
        else
            usb_sendstall();
        break;

    case STD_CLEAR_FEATURE_ZERO:
        usb_sendstall();
        break;

    case STD_CLEAR_FEATURE_INTERFACE:
        usb_sendzlp();
        break;

    case STD_CLEAR_FEATURE_ENDPOINT:
        wIndex &= 0x0F;
        if ((wValue == 0) && wIndex && (wIndex <= 3)) {
            if (wIndex == 1)
                pUdp->UDP_CSR[1] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT);
            else if (wIndex == 2)
                pUdp->UDP_CSR[2] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN);
            else if (wIndex == 3)
                pUdp->UDP_CSR[3] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_IN);
            usb_sendzlp();
        }
        else
            usb_sendstall();
        break;

	// handle CDC class requests
    case SET_LINE_CODING:
        while ( !(pUdp->UDP_CSR[0] & AT91C_UDP_RX_DATA_BK0) );
        pUdp->UDP_CSR[0] &= ~(AT91C_UDP_RX_DATA_BK0);
        usb_sendzlp();
        break;

    case GET_LINE_CODING:
        usb_senddata((char *) &line, MIN(sizeof(line), wLength));
        break;

    case SET_CONTROL_LINE_STATE:
        currentConnection = wValue;
        usb_sendzlp();
        break;

    default:
        usb_sendstall();
        break;
    }
}

void
usb_print(char *str)
{
    int l = strlen(str);
    usb_write(str, l);
}

#define PACKSIZE	512
unsigned char buf[PACKSIZE];

int notfirsttime;

void
usb_poll(void)
{
    if (notfirsttime == 0) {
        if (usb_isconfigured())
        {
            notfirsttime = 1;
        }
    }

    if (usb_isconfigured()) {
        int length;

        length = usb_read((char*) buf, PACKSIZE);

        if (!usb_isconfigured())
            return;

        samba(buf);
    }
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
