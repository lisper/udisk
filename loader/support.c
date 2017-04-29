/*
 * support.c
 * $Id: support.c 65 2013-11-17 17:53:53Z brad $
 */

#include "main.h"

unsigned int default_radix = 10;

void cat_int_radix(char *s, int i, int radix);

int out_raw_ptr(char *s, int len, char *ptr, int plen)
{
    s += len;
    len += plen;
    while (plen--)
        *s++ = *ptr++;
    return len;
}

int out_raw(char *s, int len, int l, unsigned int w)
{
    s += len;
    switch (l) {
    case 1:
        s[0] = w;
        len++;
        break;
    case 2:
        s[1] = w >> 8;
        s[0] = w;
        len += 2;
        break;
    case 4:
        s[3] = w >> 24;
        s[2] = w >> 16;
        s[1] = w >> 8;
        s[0] = w;
        len += 4;
        break;
    }
    return len;
}

void cat_chr(char *s, char c)
{
    while (*s != 0) {
        s++;
    }
    s[0] = c;
    s[1] = 0;
}


void cat_str(char *s, const char *text)
{
    while (*s != 0) {
        s++;
    }
    while (*text != 0) {
        *s++ = *text++;
    }
    *s = 0;
}


// Example output:
//   "000001FF" (Output is zero-prepadded)
//   "FFFFFFFF" (Output is unsigned)
void cat_hex(char *s, unsigned int i)
{
    int j;
    int k;

    while (*s != 0) {
        s++;
    }
    for (j = 28; j >= 0; j -= 4) {
        k = (i >> j) & 15;
        if (k <= 9) {
            *s = '0' + k;
        } else {
            *s = 'A' + k - 10;
        }
        s++;
    }
    *s = 0;
}


void cat_int(char *s, int i)
{
    cat_int_radix(s, i, 10);
}


/*
 * output radix: 2 to 36
 *        Add 100 for uppercase letter-digits.
 *        Negate the number to include radix prefix ("0x1ff").
 *        0: Use default radix.
 */
void cat_int_radix(char *s, int i, int radix)
{
    int show_radix_prefix = 0;
    int uppercase = 0;
    int j;
    char c;
    char outstr[33];
    char *o = outstr + 32;
    outstr[32] = 0;

    if (radix == 0) {
        radix = default_radix;
    }
    if (radix < 0) {
        radix = -radix;
        show_radix_prefix = 1;
    }
    if (radix >= 100) {
        radix -= 100;
        uppercase = 1;
    }

    if (i < 0) {
        i = -i;                     // Note: Will fail on the largest negative number.
        cat_str(s, "-");
    }

    if (show_radix_prefix) {
        if (radix == 10) {
            cat_str(s, "");
        } else if (radix == 2) {
            cat_str(s, "0b");
        } else if (radix == 8) {
            cat_str(s, "0o");
        } else if (radix == 16) {
            cat_str(s, "0x");
        } else {
            cat_str(s, "radix");
            cat_int_radix(s, radix, 10);
            cat_str(s, "_");
        }
    }

    do {
        j = (unsigned)i % radix;
        i = (unsigned)i / radix;
        if (j <= 9) {
            c = '0' + j;
        } else {
            if (uppercase) {
                c = 'A' + j - 10;
            } else {
                c = 'a' + j - 10;
            }
        }
        *--o = c;
    } while (i);
    cat_str(s, o);
}

/*
 * str_to_int - String to int parser.
 *  * Case insensitive.
 *  * Supports negative numbers.
 *  * Overflow detection.
 *  * "0x7fffffff": OK. "0x80000000" and above: Treated as unsigned int, then
 *    cast to int. So, to read large unsigned ints, just cast the returned
 *    value to unsigned int.
 * Parameters:
 *  * s     - Input string.
 *  * s_len - Length of s. -1 means zero-terminated string.
 *  * radix - 2 to 36. -10 means 10 with 0x/0o/0b prefixing enabled.
 *  * valid - Can be NULL.
*/
int str_to_int(const char *s, int s_len, int radix, int *valid)
{
    const char *t = s;
    const char *tend;
    char c;
    int i;
    int ns = 0;
    int ns2;
    unsigned int nu = 0;
    unsigned int nu2;
    int v = 1;
    int v_signed = 1;
    int v_unsigned = 1;

    if (s_len >= 0) {
        tend = s + s_len;
    } else {
        tend = s + strlen(s);
    }

    if ((t < tend) && (t[0] == '-')) {
        t++;
    }

    if (radix == -10) {
        radix = 10;
        if ((t + 2 <= tend) && (t[0] == '0') && (t[1] == 'x')) {
            t += 2;
            radix = 16;
        } else if ((t + 2 <= tend) && (t[0] == '0') && (t[1] == 'o')) {
            t += 2;
            radix = 8;
        } else if ((t + 2 <= tend) && (t[0] == '0') && (t[1] == 'b')) {
            t += 2;
            radix = 2;
        }
    }

    if (t >= tend) {
        v = 0;
    }

    while (t < tend) {
        c = t++[0];
        if (('0' <= c) && (c <= '9')) {
            i = c - '0';
        } else if (('A' <= c) && (c <= 'Z')) {
            i = c - 'A' + 10;
        } else if (('a' <= c) && (c <= 'z')) {
            i = c - 'a' + 10;
        } else {
            i = 0;
            v = 0;
        }
        if (i >= radix) {
            v = 0;
        }
        // Add digit, with overflow detection.
        // Signed
        ns2 = ns;
        ns *= radix;
        if (((unsigned)ns / radix != ns2) || (ns < 0)) {
            v_signed = 0;
        }
        ns += i;
        if (ns < 0) {
            v_signed = 0;
        }
        // Unsigned
        nu2 = nu;
        nu *= radix;
        if (nu / radix != nu2) {
            v_unsigned = 0;
        }
        nu2 = nu;
        nu += i;
        if (nu < nu2) {
            v_unsigned = 0;
        }
    }

    if (v && v_signed) {
        if (s[0] == '-') {
            ns = -ns;
        }
    } else if (v && v_unsigned) {
        ns = (int)nu;
        if (s[0] == '-') {
            ns = 0;
            v = 0;
        }
    } else {
        ns = 0;
        v = 0;
    }

    if (!v) {
        ns = 0;
    }

    if (valid) {
        *valid = v;
    }

    return ns;
}


void
parse_two_comma_separated_hex(const char *s, unsigned int *a, unsigned int *b)
{
	const char *q;
        char *t;
	q = strchr(s, ',');
	if (q != NULL) {
		*a = str_to_int(s, q - s, 16, NULL);
                t = strchr(q+1, '#');
                if (t) *t=0;
		*b = str_to_int(q + 1, -1, 16, NULL);
	} else {
		*a = str_to_int(s, -1, 16, NULL);
		*b = 0;
	}
}


// Example: "123, 0xff, -0x100, 0b11001111".
// Returns the number of elements.
int
parse_comma_separated_ints(const char *s, int out[], int out_len, int *valid)
{
    const char *q;
    const char *r;
    int i;
    int v;
    int out_pos = 0;

    *valid = 1;

    while (s[0] == ' ')
        s++;
    if (s[0] == 0) {
        return 0;
    }

    do {
        while (s[0] == ' ')
            s++;
        q = strchr(s, ',');
        if (q != NULL) {
            r = q;
        } else {
            r = s + strlen(s);
        }
        while (r > s && r[-1] == ' ')
            r--;
        i = str_to_int(s, r - s, -10, &v);
        if (out_pos < out_len) {
            out[out_pos++] = i;
        } else {
            *valid = 0;
        }
        if (!v) {
            *valid = 0;
        }
        s = q + 1;
    } while (q != NULL);

    return out_pos;
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
