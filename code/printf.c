/*
 * printf.c
 * $Id: printf.c 55 2010-10-15 21:07:08Z brad $
 */

#include <stdarg.h>

int printf(const char *format, ...);

int
xprintf(const char *format, ...)
{
  static const char hex[] = "0123456789ABCDEF";
  char format_flag;
  unsigned int u_val, div_val, base, size;
  char *ptr;
  va_list ap;

  va_start (ap, format);
  for (;;) {
    while ((format_flag = *format++) != '%') {      // Until '%' or '\0'
      if (!format_flag) {
	va_end (ap);
	return (0);
      }
      putc(format_flag);
    }

    size = *format;
    if ('0' <= size && size <= '9') {
      size -= '0';
      format++;
    } else
      size = 0;

    switch (format_flag = *format++) {
    case 'c': format_flag = va_arg(ap,int);
    default:  putc(format_flag); continue;

    case 's':
      ptr = va_arg(ap,char *);
      while (*ptr) putc(*ptr++);
      break;

    case 'd': base = 10; div_val = 100000000; goto CONVERSION_LOOP;
//    case 'o': base = 8; div_val = 0100000000; goto CONVERSION_LOOP;
    case 'o': base = 8; div_val = 010000000000; goto CONVERSION_LOOP;
    case 'x': base = 16; div_val = 0x10000000;

    CONVERSION_LOOP:
    u_val = va_arg(ap,int);
    if (format_flag == 'd') {
      if (((int)u_val) < 0) {
	u_val = - u_val;
	putc('-');
      }
      while (div_val > 1 && div_val > u_val) div_val /= 10;
    }
    if (size > 0) {
      static int pow10[] = { 0,1,10,100,1000,10000,100000,1000000,10000000 };
      switch (base) {
      case 8:  div_val = 1 << ((size-1) * 3); break;
      case 10: div_val = pow10[size]; break;
      case 16: div_val = 1 << ((size-1) * 4); break;
      }
    }

//    if ( (u_val / div_val) >= div_val )
//        putc('*');

    do {
      putc(hex[u_val / div_val]);
      u_val %= div_val;
      div_val /= base;
    } while (div_val);
    }
  }
}

#if 0
main()
{
  xprintf("hi\n");
  xprintf("dec %d\n", 1234);
  xprintf("hex %x\n", 0x1234);
  xprintf("oct %o\n", 01234);
  xprintf("str %s\n", "1234");
}
#endif

void *memcpy(void *t, const void *f, unsigned long n)
{
  char *to = (char *)t;
  char *from = (char *)f;
  int len = n;

  while (len--)
    *to++ = *from++;

  return to;
}

void *memset(void *t, int c, int n)
{
  char *to = (char *)t;
  char *tosave = (char *)t;
  int len = n;

  while (len--)
    *to++ = c;

  return tosave;
}

char *strncpy(char *to, char *from, int len)
{
  char *dest = to;
  while (len--)
    if ((*to++ = *from++) == 0)
      break;
  return dest;
}

int strcmp(char *s1, char *s2)
{
  while (*s1 && *s2) {
    if (*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return *s1 - *s2;
}

int strncmp(char *s1, char *s2, int n)
{
  while (*s1 && *s2) {
    if (--n == 0)
      break;
    if (*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return *s1 - *s2;
}

int strlen(char *s)
{
  int len = 0;
  while (*s++) len++;
  return len;
}


unsigned long
udivmodsi4(unsigned long num, unsigned long den, int modwanted)
{
  unsigned long bit = 1;
  unsigned long res = 0;

  while (den < num && bit && !(den & (1L<<31)))
    {
      den <<=1;
      bit <<=1;
    }
  while (bit)
    {
      if (num >= den)
	{
          num -= den;
          res |= bit;
	}
      bit >>=1;
      den >>=1;
    }
  if (modwanted) return num;
  return res;
}

long
__udivsi3 (long a, long b)
{
  return udivmodsi4 (a, b, 0);
}

long
__umodsi3 (long a, long b)
{
  return udivmodsi4 (a, b, 1);
}


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
