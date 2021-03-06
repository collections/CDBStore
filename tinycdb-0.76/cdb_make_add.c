/* $Id: cdb_make_add.c,v 1.8 2006/06/28 17:49:21 mjt Exp $
 * basic cdb_make_add routine
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 */

/*  Enhanced by Jens Alfke, Feb 2008, based on tinycdb 0.76 sources.
    Added cdb_make_addv() to allow a value to be specified in multiple memory blocks,
    in the same manner as the POSIX writev function.
*/

#include <stdlib.h> /* for malloc */
#include "cdb_int.h"

static int
_cdb_make_addv(struct cdb_make *cdbmp, unsigned hval,
               const void *key, unsigned klen,
               const struct cdb_iovec *vals, unsigned nvals)
{
  unsigned vlen = 0;
  unsigned char rlen[8];
  struct cdb_rl *rl;
  unsigned i;
  
  for (i=0; i<nvals; i++)
    vlen += vals[i].cdb_iov_len;
  if (klen > 0xffffffff - (cdbmp->cdb_dpos + 8) ||
      vlen > 0xffffffff - (cdbmp->cdb_dpos + klen + 8))
    return errno = ENOMEM, -1;
  i = hval & 255;
  rl = cdbmp->cdb_rec[i];
  if (!rl || rl->cnt >= sizeof(rl->rec)/sizeof(rl->rec[0])) {
    rl = (struct cdb_rl*)malloc(sizeof(struct cdb_rl));
    if (!rl)
      return errno = ENOMEM, -1;
    rl->cnt = 0;
    rl->next = cdbmp->cdb_rec[i];
    cdbmp->cdb_rec[i] = rl;
  }
  i = rl->cnt++;
  rl->rec[i].hval = hval;
  rl->rec[i].rpos = cdbmp->cdb_dpos;
  ++cdbmp->cdb_rcnt;
  cdb_pack(klen, rlen);
  cdb_pack(vlen, rlen + 4);
  if (_cdb_make_write(cdbmp, rlen, 8) < 0 ||
      _cdb_make_write(cdbmp, key, klen) < 0)
    return -1;
  for (i=0; i<nvals; i++)
    if (_cdb_make_write(cdbmp, vals[i].cdb_iov_base, vals[i].cdb_iov_len) < 0)
      return -1;
  return 0;
}



int internal_function
_cdb_make_add(struct cdb_make *cdbmp, unsigned hval,
              const void *key, unsigned klen,
              const void *val, unsigned vlen) {
  struct cdb_iovec vals = {val,vlen};
  return _cdb_make_addv(cdbmp,hval,key,klen, &vals,1);
}

int
cdb_make_add(struct cdb_make *cdbmp,
             const void *key, unsigned klen,
             const void *val, unsigned vlen) {
  return _cdb_make_add(cdbmp, cdb_hash(key, klen), key, klen, val, vlen);
}

int cdb_make_addv(struct cdb_make *cdbmp,
                  const void *key, unsigned klen,
                  const struct cdb_iovec *vals, unsigned nvals) {
  return _cdb_make_addv(cdbmp, cdb_hash(key, klen), key, klen, vals, nvals);
}