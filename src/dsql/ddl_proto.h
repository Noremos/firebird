/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		ddl_proto.h
 *	DESCRIPTION:	Prototype Header file for ddl.cpp
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
 *
 * 2004.01.16 Vlad Horsun: added support for default parameters and
 *   EXECUTE BLOCK statement
 */

#ifndef DSQL_DDL_PROTO_H
#define DSQL_DDL_PROTO_H

// This is a DSQL internal file. Not to be used by anything but
// the DSQL module itself.

namespace Jrd {
	class DsqlCompilerScratch;
	class dsql_fld;
};

const USHORT blr_dtypes[] = {
	0,
	blr_text,					// dtype_text
	blr_cstring,				// dtype_cstring
	blr_varying,				// dtype_varying
	0,
	0,
	0,							// dtype_packed
	0,							// dtype_byte
	blr_short,					// dtype_short
	blr_long,					// dtype_long
	blr_quad,					// dtype_quad
	blr_float,					// dtype_real
	blr_double,					// dtype_double
	blr_double,					// dtype_d_float
	blr_sql_date,				// dtype_sql_date
	blr_sql_time,				// dtype_sql_time
	blr_timestamp,				// dtype_timestamp
	blr_blob,					// dtype_blob		// ASF: CAST use blr_blob2 because blr_blob doesn't fit in UCHAR
	blr_short,					// dtype_array
	blr_int64,					// dtype_int64
	0,							// DB_KEY
	blr_bool,					// dtype_boolean
	blr_dec64,					// dtype_dec64
	blr_dec128,					// dtype_dec128
	blr_int128,					// dtype_int128
	blr_sql_time_tz,			// dtype_sql_time_tz
	blr_timestamp_tz,			// dtype_timestamp_tz
	blr_ex_time_tz,				// dtype_ex_time_tz
	blr_ex_timestamp_tz			// dtype_ex_timestamp_tz
};

bool DDL_ids(const Jrd::DsqlCompilerScratch*);
void DDL_resolve_intl_type(Jrd::DsqlCompilerScratch*, Jrd::dsql_fld*, Jrd::QualifiedName&,
	bool = false);

#endif // DSQL_DDL_PROTO_H
