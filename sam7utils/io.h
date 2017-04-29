/*
 * io.h
 *
 * Copyright (C) 2005 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#ifndef __io_h__
#define __io_h__


#include <unistd.h>  // for usleep


int io_init( char *dev );
int io_cleanup( void );
int io_write( void *buff, int len );
int io_read( void *buff, int len );
int io_read_response( char *buff, int len );

extern int samba_interactive;
extern void dump_io(char *name, char *buf, int buflen);

static inline int io_send_cmd( char *cmd, void *response, int response_len )
{
  int ret;

  dump_io("out", cmd, strlen(cmd));

  if( io_write( cmd, strlen( cmd ) ) < 0 ) {
    return -1;
  }
  usleep( 2000 );

  if( response_len == 0 ) {
    return 0;
  }

  if (samba_interactive) {
    /* interactive mode */
    char buf[128];
    int i;
    char *d;

    ret = io_read( buf, sizeof(buf) );
    if( ret < 0 ) {
      return -1;
    }

    dump_io("in ", buf, ret);

    d = (char *)response + response_len - 1;

    if (buf[0] == '0' && buf[1] == 'x') {
      /* interpret hex response */
      for (i = 0; i < ret && response_len > 0; i++) {
        char *p = &buf[i];
        char b[3]; int n;

        b[0] = p[0]; b[1] = p[1]; b[2] = 0;
        n = 0;
        sscanf(b, "%x", &n);
		  
        *d-- = n;
        response_len--;
        if (0) printf("%d: %s %x\n", i, b, n);
        i++;
      }
    } else {
      /* raw bytes */
      for (i = 0; i < ret && response_len > 0; i++) {
        ((char *)response)[i] = buf[i];
        response_len--;
      }
    }
  } else {
    /* non-interactive */
    ret = io_read( response, response_len );
    if (ret < 0)  {
      return -1;
    }

    dump_io("in ", response, ret);
    ((char *)response)[ret] = 0;
  }

  usleep( 2000 );
  return 0;
}

#endif /* __io_h__ */


/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:2
 * End:
*/
