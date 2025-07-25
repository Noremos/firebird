/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ini.h
 *	DESCRIPTION:	Declarations for metadata initialization
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

/* Note that this file is used by jrdmet.cpp in gpre
   as well as by ini.epp in JRD.  Make sure that any
   changes are compatible in both places. */

#ifndef JRD_INI_H
#define JRD_INI_H

#include "../common/intlobj_new.h"
#include "../jrd/intl.h"
#include "../intl/country_codes.h"
#include "../intl/charsets.h"
#include "../jrd/obj.h"
#include "../jrd/dflt.h"
#include "../jrd/constants.h"
#include "../jrd/ods.h"

//******************************
// names.h
//******************************

// Define name ids

#define NAME(name, id) id,

enum name_ids
{
	nam_MIN,
#include "../jrd/names.h"
	nam_MAX
};

#undef NAME

// Define name strings

#define NAME(name, id) name,

static inline constexpr const TEXT* names[] =
{
	0,
#include "../jrd/names.h"
};
#undef NAME

//******************************
// fields.h
//******************************
inline constexpr USHORT BLOB_SIZE			= 8;
inline constexpr USHORT TIMESTAMP_SIZE		= 8;
inline constexpr USHORT TIMESTAMP_TZ_SIZE	= 12;

// Pick up global ids


#define FIELD(type, name, dtype, length, sub_type, dflt_blr, nullable, ods)	type,
enum gflds
{
#include "../jrd/fields.h"
	gfld_MAX
};
#undef FIELD

typedef gflds GFLDS;

// Pick up actual global fields

#ifndef GPRE
#define FIELD(type, name, dtype, length, sub_type, dflt_blr, nullable, ods)	\
	{ (int) type, (int) name, dtype, length, sub_type, dflt_blr, sizeof(dflt_blr), nullable, ods },
#else
#define FIELD(type, name, dtype, length, sub_type, dflt_blr, nullable, ods)	\
	{ (int) type, (int) name, dtype, length, sub_type, NULL, 0, true, ods },
#endif

struct gfld
{
	int				gfld_type;
	int				gfld_name;
	UCHAR			gfld_dtype;
	USHORT			gfld_length;
	SSHORT			gfld_sub_type;
	const UCHAR*	gfld_dflt_blr;
	USHORT			gfld_dflt_len;
	bool			gfld_nullable;
	USHORT			gfld_ods_version;
};

static inline constexpr struct gfld gfields[] =
{
#include "../jrd/fields.h"
	{ 0, 0, dtype_unknown, 0, 0, NULL, 0, false, 0 }
};
#undef FIELD

//******************************
// relations.h
//******************************

// Pick up relation ids

#define RELATION(name, id, ods, type) id,
#define FIELD(symbol, name, id, update, ods)
#define END_RELATION
enum rids
{
#include "../jrd/relations.h"
	rel_MAX
};
#undef RELATION
#undef FIELD
#undef END_RELATION

typedef rids RIDS;

// Pick up relations themselves

#define RELATION(name, id, ods, type)	(int) name, (int) id, ods, type,
#define FIELD(symbol, name, id, update, ods)\
				(int) name, (int) id, update, (int) ods,
#define END_RELATION		0,

inline constexpr int RFLD_R_NAME	= 0;
inline constexpr int RFLD_R_ID		= 1;
inline constexpr int RFLD_R_ODS		= 2;
inline constexpr int RFLD_R_TYPE	= 3;
inline constexpr int RFLD_RPT		= 4;

inline constexpr int RFLD_F_NAME	= 0;
inline constexpr int RFLD_F_ID		= 1;
inline constexpr int RFLD_F_UPDATE	= 2;
inline constexpr int RFLD_F_ODS		= 3;
inline constexpr int RFLD_F_LENGTH	= 4;

static inline constexpr int relfields[] =
{
#include "../jrd/relations.h"
	0
};

#undef RELATION
#undef FIELD
#undef END_RELATION

//******************************
// SystemPrivileges.h
//	should go before types.h
//******************************

#include "SystemPrivileges.h"

//******************************
// types.h
//******************************

// obtain field types

struct rtyp
{
	const TEXT* rtyp_name;
	SSHORT rtyp_value;
	int rtyp_field;
};

#define TYPE(text, type, field)	{ text, type, field },

static inline constexpr rtyp types[] =
{
#include "../jrd/types.h"
	{NULL, 0, 0}
};

#undef TYPE

#endif	// JRD_INI_H
