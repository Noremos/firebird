/*
 *	PROGRAM:		Firebird CONSTANTS implementation.
 *	MODULE:			Package.h
 *	DESCRIPTION:	Routine to cache and reload Package constants
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Artyom Abakumov
 *  <artyom.abakumov (at) red-soft.ru> for Red Soft Corporation.
 *
 *  Copyright (c) 2025 Red Soft Corporation <info (at) red-soft.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */


#ifndef JRD_CONSTANT_H
#define JRD_CONSTANT_H

#include "firebird.h"
#include "../jrd/CacheVector.h"
#include "../jrd/Resources.h"
#include "../jrd/obj.h"
#include "../jrd/val.h"
#include "../jrd/lck.h"
#include "../common/classes/GenericMap.h"

namespace Jrd
{
class DsqlCompilerScratch;
class dsql_fld;

class ConstantValue final : public Firebird::PermanentStorage
{
public:
	ConstantValue(MemoryPool& pool) :
		Firebird::PermanentStorage(pool),
		name(pool)
	{ }

	MetaId id{};
	QualifiedName name;

	// Keep type to gen hash (when not commited - we cannot read it from system table)
	// Keep value when scanning and after the first execution
	impure_value value{};

	// keep only materialized value
	bid blrBlobId{};

	bool isPrivate = false;

	bool hash(thread_db* tdbb, Firebird::sha512& digest) const;

	static dsc getDesc(thread_db* tdbb, Jrd::jrd_tra* transaction, const QualifiedName& name);
	static dsc getDesc(thread_db* tdbb, Jrd::jrd_tra* transaction, const MetaId id);

	static void genConstantBlr(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
		ValueExprNode* constExpr, dsql_fld* type, const MetaName& schema);

	dsc& makeValue(thread_db* tdbb);

	~ConstantValue()
	{
		delete value.vlu_string;
	}
};

struct ConstantsCache
{
	using ValueId = ULONG;

	ConstantsCache(MemoryPool& pool) :
		idMap(pool),
		nameMap(pool),
		values(pool)
	{ }

	Firebird::NonPooledMap<MetaId, ValueId> idMap;
	Firebird::LeftPooledMap<QualifiedName, ValueId> nameMap;

	Firebird::ObjectsArray<ConstantValue> values;

	ConstantValue& add(const MetaId constId, const QualifiedName& constName, const bool isPrivate);

	void clear()
	{
		idMap.clear();
		nameMap.clear();
		values.clear();
	}
};


class PackagePermanent : public Firebird::PermanentStorage
{
public:
	explicit PackagePermanent(thread_db* tdbb, MemoryPool& p, MetaId metaId, NoData)
		: PermanentStorage(p),
			id(metaId),
			name(p)
	{ }

	explicit PackagePermanent(MemoryPool& p)
		: PermanentStorage(p),
			id(~0),
			name(p)
	{ }

	MetaId getId() const
	{
		return id;
	}

	static bool destroy(thread_db* tdbb, PackagePermanent* routine)
	{
		return false;
	}

	void releaseLock(thread_db*) { }

	const QualifiedName& getName() const noexcept { return name; }
	void setName(const QualifiedName& value) { name = value; }

	bool hasData() const { return name.hasData(); }

public:
	MetaId id;							// routine ID
	QualifiedName name;					// routine name
};

class Package final : public Firebird::PermanentStorage, public ObjectBase
{
public:
	// lock requeued by CacheElement
	static const enum lck_t LOCKTYPE = LCK_package_rescan;

private:
	explicit Package(Cached::Package* perm)
		: Firebird::PermanentStorage(perm->getPool()),
			constants(perm->getPool()),
			cachedPackage(perm)
	{ }

public:
	explicit Package(MemoryPool& p)
		: Firebird::PermanentStorage(p),
		  constants(p)
	{ }

	// Methods needed by the MetaCache
	// ----------

	static bool destroy(thread_db* tdbb, Package* routine)
	{
		return false;
	}

	static Package* create(thread_db* tdbb, MemoryPool& pool, Cached::Package* perm);
	static std::optional<MetaId> getIdByName(thread_db* tdbb, const QualifiedName& name);

	ScanResult scan(thread_db* tdbb, ObjectBase::Flag flags);
	void checkReload(thread_db* tdbb);
	ScanResult reload(thread_db* tdbb, ObjectBase::Flag flags);

	static const char* objectFamily(void*)
	{
		return "package";
	}

	MetaId getId() const
	{
		return getPermanent()->id;
	}

	int getObjectType() const noexcept
	{
		return objectType();
	}

	SLONG getSclType() const noexcept
	{
		return obj_package_header;
	}

	static int objectType();

	bool hash(thread_db* tdbb, Firebird::sha512& digest);

	Cached::Package* getPermanent() const noexcept
	{
		return cachedPackage;
	}

	// ----------

	ConstantValue& addConstant(thread_db* tdbb, const MetaId constId,
		const QualifiedName& constName,
		const bool isPrivate,
		const TypeClause* type);

	ConstantValue& addConstant(thread_db* tdbb, const MetaId constId,
		const QualifiedName& constName,
		const bool isPrivate,
		const bid blrBlobId,
		const bool skipMakeValue = false);

	ConstantValue* findConstant(thread_db* tdbb, const MetaId id);
	ConstantValue* findConstant(thread_db* tdbb, QualifiedName name);

private:
	virtual ~Package() = default;

private:
	ConstantsCache constants;
	Cached::Package* cachedPackage = nullptr;		// entry in the cache
	bool m_callReload = true;
};

} // namespace Jrd

#endif // JRD_CONSTANT_H
