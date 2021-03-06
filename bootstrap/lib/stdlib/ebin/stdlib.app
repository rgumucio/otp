%% This is an -*- erlang -*- file.
%% 
%% %CopyrightBegin%
%%
%% Copyright Ericsson AB 2008-2010. All Rights Reserved.
%%
%% The contents of this file are subject to the Erlang Public License,
%% Version 1.1, (the "License"); you may not use this file except in
%% compliance with the License. You should have received a copy of the
%% Erlang Public License along with this software. If not, it can be
%% retrieved online at http://www.erlang.org/.
%%
%% Software distributed under the License is distributed on an "AS IS"
%% basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
%% the License for the specific language governing rights and limitations
%% under the License.
%%
%% %CopyrightEnd%
%%
{application, stdlib,
 [{description, "ERTS  CXC 138 10"},
  {vsn, "1.17"},
  {modules, [array,
	     base64,
	     beam_lib,
	     c,
	     calendar,
	     dets,
             dets_server,
	     dets_sup,
	     dets_utils,
	     dets_v8,
	     dets_v9,
	     dict,
	     digraph,
	     digraph_utils,
	     edlin,
	     edlin_expand,
	     epp,
	     eval_bits,
	     erl_bits,
	     erl_compile,
	     erl_eval,
             erl_expand_records,
	     erl_internal,
	     erl_lint,
	     erl_parse,
	     erl_posix_msg,
	     erl_pp,
	     erl_scan,
	     erl_tar,
	     error_logger_file_h,
	     error_logger_tty_h,
	     escript,
	     ets,
	     file_sorter,
	     filelib,
	     filename,
	     gb_trees,
	     gb_sets,
	     gen,
	     gen_event,
	     gen_fsm,
	     gen_server,
	     io,
	     io_lib,
	     io_lib_format,
	     io_lib_fread,
	     io_lib_pretty,
	     lib,
	     lists,
	     log_mf_h,
	     math,
	     ms_transform,
	     orddict,
	     ordsets,
	     otp_internal,
	     pg,
	     pool,
	     proc_lib,
	     proplists,
             qlc,
             qlc_pt,
	     queue,
	     random,
	     re,
	     regexp,
	     sets,
	     shell,
	     shell_default,
	     slave,
	     sofs,
	     string,
	     supervisor,
	     supervisor_bridge,
	     sys,
	     timer,
	     unicode,
	     win32reg,
	     zip]},
  {registered,[timer_server,rsh_starter,take_over_monitor,pool_master,
               dets]},
  {applications, [kernel]},
  {env, []}]}.

