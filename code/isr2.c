/*
 * handle interrupt from cpld - addresss match
 */
void
cpld_isr(void)
{
    AT91PS_PIO pPio;
    unsigned short sigs, data, addr_lo;

    pPio = AT91C_BASE_PIOA;
    cpld_ints++;

#if 1
    cpld_write_sub(CPLD_SUBREG_INT_RESET, 0);
    {
        volatile u32 v;
    *AT91C_AIC_ICCR = 0xffffffff;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
    }
#endif

    /* sample C0,C1 */
    sigs = cpld_read(CPLD_REG_STATUS);

 restart:
    /* low bits of matching address */
    addr_lo = cpld_read(CPLD_REG_RD_ADDR_LO);

#if 1
    /* short circuit */
    if ((sigs & (CV_BUS_C1 | CV_BUS_C0)) == 0 && (addr_lo & 077) == CS) {
        volatile u32 v;
        reads_cs++;
        cpld_write(CPLD_REG_DATA, cs);
        cpld_write_sub(CPLD_SUBREG_CPU_RELEASE, 1);

        cpld_write_sub(CPLD_SUBREG_INT_RESET, 0);
        *AT91C_AIC_ICCR = 0xffffffff;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
        return;
    }
#endif

cpld_write_sub(CPLD_SUBREG_INT_RESET, 0);

#if 1
    /* 18 bit address on unibus */
    if (cpld_read(CPLD_REG_RD_ADDR_HI) != 03) {
        bad_match++;
        goto done;
    }
#endif

    /* read? */
    if ((sigs & CV_BUS_C1) == 0) {
        switch (addr_lo & 016) {
        case CS:
            update_cs();
            data = cs;
reads_cs++;
            break;
        case BA:
            data = ba & 0177776;
            break;
        case DA:
            data = da;
            break;
        case MP:
            data = mp[0];
            mp[0] = mp[1];
            mp[1] = mp[2];
            break;
        }

        cpld_write(CPLD_REG_DATA, data);
        /* done */
logw(1, addr_lo & 0x16, data);
    }

    /* catch DATIP (r-m-w) */
    if ((sigs & (CV_BUS_C0 | CV_BUS_C1)) == (CV_BUS_C0)) {

        /* release first half */
        cpld_write_sub(CPLD_SUBREG_CPU_RELEASE, 1);

        /* wait for state machine to cycle */
        do {
            sigs = cpld_read(CPLD_REG_STATUS);
        } while (sigs & (1 << 8));

        /* wait for new msyn */
	while ((sigs & (1 << 8)) == 0) {
            sigs = cpld_read(CPLD_REG_STATUS);
        }

        rmws++;
    }

    /* write? */
    if (sigs & CV_BUS_C1) {
        /* write; sample the data bus */
        data = cpld_read(CPLD_REG_DATA);

logw(2, addr_lo & 0x16, data);
        switch (addr_lo & 017) {
        case CS:
            /* honor byte writes */
            if (sigs & CV_BUS_C0)
                data = byte_place(addr_lo, cs, data);

            cs = (cs & ~CS_RW) | (data & CS_RW);
            update_cs();
writes_cs++;

            /* write CRDY=1 */
            if (data & CS_CRDY) {
                if ((data & CS_IE) == 0) {
//                    int_pending = 0;
                } else
                    /* writing RDY & IE - if we were RDY, gen int */
                    if ((cs & (CS_CRDY | CS_IE)) == CS_CRDY) {
                        int_pending = 1;
                    }
                break;
            }

            /* wrote CRDY=0; clear errors */
            int_pending = 0;
            cs &= ~CS_ANY_ERR;

            cmd_pending = 1;
            break;

        case BA:
            if (sigs & CV_BUS_C0)
                data = byte_place(addr_lo, ba, data);
            ba = data & 0177776;
            break;
        case DA:
            if (sigs & CV_BUS_C0)
                data = byte_place(addr_lo, da, data);
            da = data;
            break;
        case MP:
            if (sigs & CV_BUS_C0)
                data = byte_place(addr_lo, mp[0], data);
            mp[0] = mp[1] = mp[2] = data;
            break;
        }
    }

    cpld_write_sub(CPLD_SUBREG_CPU_RELEASE, 1);

done:
//cpld_write_sub(CPLD_SUBREG_INT_RESET, 0);
    *AT91C_AIC_ICCR = 0xffffffff;
    {
        volatile u32 v;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
    }
}
