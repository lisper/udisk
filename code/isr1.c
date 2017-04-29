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

    cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
    *AT91C_AIC_ICCR = 0xffffffff;
    {
        volatile u32 v;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
    }

    /* sample C0,C1 */
    sigs = cpld_read(CPLD_REG_STATUS);

 restart:
    /* low bits of matching address */
    addr_lo = cpld_read(CPLD_REG_RD_ADDR_LO);

#if 1
    if (addr_lo & 070) {
        bad_match++;
        goto done;
    }
#endif

#if 1
    /* short circuit */
    if ((sigs & (CV_BUS_C1 | CV_BUS_C0)) == 0 && (addr_lo & 077) == CS) {
        volatile u32 v;
        reads_cs++;
        cpld_write(CPLD_REG_DATA, cs);
        cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);

        cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
        *AT91C_AIC_ICCR = 0xffffffff;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
        return;
    }
#endif

#if 1
    /* 18 bit address on unibus */
    if (cpld_read(CPLD_REG_RD_ADDR_HI) != 03) {
        bad_match++;
        goto done;
    }
#endif

restart2:
    if (sigs & CV_BUS_C1) {

        /* write; sample the data bus */
        data = cpld_read(CPLD_REG_DATA);

logw(2, addr_lo & 0x16, data);
        switch (addr_lo & 017) {
        case CS:
//logw(1, sigs, data);
            /* honor byte writes */
            if (sigs & CV_BUS_C0)
                data = byte_place(addr_lo, cs, data);

//logw(2, cs, data);
//if ((cs & CS_IE) && (data & CS_IE) == 0) logw(4, cs, data);
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

//if (data & CS_IE) logw(3, cs, data);
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
//logw(1, cs, data);
            break;
        case MP:
            if (sigs & CV_BUS_C0)
                data = byte_place(addr_lo, mp[0], data);
            mp[0] = mp[1] = mp[2] = data;
            break;
        }
    } else {
        /* read */
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
//logw(5, sigs, data);
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

    /* catch DATIP */
    if ((sigs & (CV_BUS_C0 | CV_BUS_C1)) == (CV_BUS_C0)) {

        /* release first half */
        cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);

        /* wait for state machine to cycle */
	sigs = cpld_read(CPLD_REG_STATUS);
//logw(2, addr_lo, sigs);

	while (sigs & (1 << 8)) {
            sigs = cpld_read(CPLD_REG_STATUS);
        }

        /* wait for new msyn */
	while ((sigs & (1 << 8)) == 0) {
            sigs = cpld_read(CPLD_REG_STATUS);
        }
//logw(3, addr_lo, sigs);

        rmws++;

//cpld_write_subreg(CPLD_SUBREG_INT_RESET, 0);
goto restart2;
	//goto restart;
    }

    cpld_write_subreg(CPLD_SUBREG_CPU_RELEASE, 1);

done:
    *AT91C_AIC_ICCR = 0xffffffff;
    {
        volatile u32 v;
        v = *AT91C_AIC_IPR;
        v = *AT91C_PIOA_ISR;
    }
}
