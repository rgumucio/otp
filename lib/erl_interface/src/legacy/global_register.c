/*
 * %CopyrightBegin%
 * 
 * Copyright Ericsson AB 1998-2009. All Rights Reserved.
 * 
 * The contents of this file are subject to the Erlang Public License,
 * Version 1.1, (the "License"); you may not use this file except in
 * compliance with the License. You should have received a copy of the
 * Erlang Public License along with this software. If not, it can be
 * retrieved online at http://www.erlang.org/.
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 * 
 * %CopyrightEnd%
 */
#include <string.h>
#include "eidef.h"
#include "eiext.h"
#include "eisend.h"
#include "eirecv.h"
#include "erl_interface.h"

int erl_global_register(int fd, const char *name, ETERM *pid)
{
  char buf[EISMALLBUF];
  char *bufp=buf;
  char tmpbuf[64];
  int index = 0;
  erlang_pid self;
  erlang_msg msg;
  int needlink, needatom;
  int arity;
  int version;
  int msglen;
  int i;

  /* get that pid into a better format */
  if (!erl_encode(pid,(unsigned char*)buf)) return -1;
  if (ei_decode_version(buf,&index,&version)
      || ei_decode_pid(buf,&index,&self)) return -1;
  
  /* set up rpc arguments */
  /* { PidFrom, { call, Mod, Fun, Args, user }}  */
  index = 0;
  ei_encode_version(buf,&index);
  ei_encode_tuple_header(buf,&index,2);
  ei_encode_pid(buf,&index,&self);               /* PidFrom */
  ei_encode_tuple_header(buf,&index,5);
  ei_encode_atom(buf,&index,"call");            /* call */
  ei_encode_atom(buf,&index,"global");          /* Mod */
  ei_encode_atom(buf,&index,"register_name_external");    /* Fun */
  ei_encode_list_header(buf,&index,3);     /* Args: [ name, self(), cnode ] */
  ei_encode_atom(buf,&index,name);
  ei_encode_pid(buf,&index,&self); 
  ei_encode_tuple_header(buf,&index,2);
  ei_encode_atom(buf,&index,"global"); /* special "resolve" treatment */ 
  ei_encode_atom(buf,&index,"cnode");  /* i.e. we get a SEND when conflict */
  ei_encode_empty_list(buf,&index);
  ei_encode_atom(buf,&index,"user");            /* user */

  /* make the rpc call */
  if (ei_send_reg_encoded(fd,&self,"rex",buf,index)) return -1;

  /* get the reply: expect link and an atom, or just an atom */
  needlink = needatom = 1;
  while (1) {
    /* get message */
    while (1) {
      index = EISMALLBUF;
      if (!(i = ei_recv_internal(fd,&bufp,&index,&msg,&msglen,1,0))) continue;
      else break;
    }

    switch (i) {
    case ERL_LINK:
      /* got link */
      if (!needlink) return -1;
	needlink = 0;
      break;
      
    case ERL_SEND:
      /* got message - does it contain our atom? */
      if (!needatom) return -1;
      else {
	/* expecting { rex, yes } */
	index = 0;
	if (ei_decode_version(buf,&index,&version) 
	    || ei_decode_tuple_header(buf,&index,&arity) 
	    || (arity != 2) 
	    || ei_decode_atom(buf,&index,tmpbuf) 
	    || strcmp(tmpbuf,"rex")
	    || ei_decode_atom(buf,&index,tmpbuf) 
	    || strcmp(tmpbuf,"yes"))
	  return -1; /* bad response from other side */

	/* we're done */
	return 0;
      }
      break;
      
    default:
      return -1; /* something else */
    }
  }
  return 0;
}

