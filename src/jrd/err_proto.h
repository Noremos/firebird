/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		err_proto.h
 *	DESCRIPTION:	Prototype header file for err.cpp
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#ifndef JRD_ERR_PROTO_H
#define JRD_ERR_PROTO_H

#include "fb_exception.h"
#include "../common/classes/fb_string.h"
#include "../jrd/MetaName.h"
#include "../common/StatusArg.h"
#include "../jrd/status.h"

namespace Jrd {

// Index error types

enum idx_e {
	idx_e_ok = 0,
	idx_e_duplicate,
	idx_e_keytoobig,
	idx_e_conversion,
	idx_e_interrupt,
	idx_e_foreign_target_doesnt_exist,
	idx_e_foreign_references_present
};

} //namespace Jrd

void	ERR_post_warning(const Firebird::Arg::StatusVector& v);
//void	ERR_assert(const TEXT*, int);
[[noreturn]] void ERR_bugcheck(int, const TEXT* = NULL, int = 0);
[[noreturn]] void ERR_bugcheck_msg(const TEXT*);
[[noreturn]] void ERR_soft_bugcheck(int, const TEXT*, int);
[[noreturn]] void ERR_corrupt(int);
[[noreturn]] void ERR_error(int);
[[noreturn]] void ERR_post(const Firebird::Arg::StatusVector& v);
void	ERR_post_nothrow(const Firebird::Arg::StatusVector& v, Jrd::FbStatusVector* statusVector = NULL);
void	ERR_post_nothrow(const Firebird::IStatus* v, Jrd::FbStatusVector* statusVector = NULL);
[[noreturn]] void ERR_punt();
void	ERR_warning(const Firebird::Arg::StatusVector& v);
void	ERR_log(int, int, const TEXT*);
void	ERR_append_status(Jrd::FbStatusVector*, const Firebird::Arg::StatusVector& v);
void	ERR_build_status(Jrd::FbStatusVector*, const Firebird::Arg::StatusVector& v) noexcept;

#endif // JRD_ERR_PROTO_H
