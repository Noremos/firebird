/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tra.h
 *	DESCRIPTION:	Transaction block definitions
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
 * 2001.6.25 Claudio Valderrama: add dfw_delete_generator and dfw_delete_udf
 *           to the dfw_t enumeration.
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 */

#ifndef JRD_TRA_H
#define JRD_TRA_H

/*
 * TMN: Fix this header! It should include any header it needs
 * to define the types this header uses.
 */

#include "../include/fb_blk.h"
#include "../common/classes/tree.h"
#include "../common/classes/GenericMap.h"
#include "../jrd/exe.h"
#include "../jrd/rpb_chain.h"
#include "../jrd/blb.h" // For bid structure
#include "../jrd/sbm.h" // For bid structure
#include "../jrd/sort.h"

#include "../jrd/Monitoring.h"
#include "../jrd/TempSpace.h"
#include "../jrd/obj.h"
#include "../jrd/EngineInterface.h"
#include "../jrd/Savepoint.h"

namespace EDS {
class Transaction;
}

namespace Jrd {

class blb;
class Lock;
class jrd_rel;
template <typename T> class vec;
class Record;
class ArrayField;
class Attachment;
class DeferredWork;
class DeferredJob;
class TimeZoneSnapshot;
class UserManagement;
class MappingList;
class DbCreatorsList;
class thread_db;

class SecDbContext
{
public:
	SecDbContext(Firebird::IAttachment* a, Firebird::ITransaction* t);
	~SecDbContext();

	Firebird::IAttachment* att;
	Firebird::ITransaction* tra;
	int savePoint;
};

// Blobs active in transaction identified by bli_temp_id. Please keep this
// structure small as there can be huge amount of them floating in memory.
struct BlobIndex
{
	ULONG bli_temp_id;
	bool bli_materialized = false;
	Request* bli_request = nullptr;
	union
	{
		bid bli_blob_id;		// ID of materialized blob
		blb* bli_blob_object;	// Blob object
	};
    static const ULONG& generate(const void* /*sender*/, const BlobIndex& item) noexcept
	{
		return item.bli_temp_id;
    }
	// Empty default constructor to make it behave like POD structure
	BlobIndex() noexcept : bli_temp_id(0), bli_blob_object(nullptr) {}
	BlobIndex(ULONG temp_id, blb* blob_object) noexcept :
		bli_temp_id(temp_id), bli_blob_object(blob_object)
	{ }
};

typedef Firebird::BePlusTree<BlobIndex, ULONG, BlobIndex> BlobIndexTree;
typedef Firebird::BePlusTree<bid, bid> FetchedBlobIdTree;

// Transaction block

struct CallerName
{
	CallerName(int aType, const QualifiedName& aName, const Firebird::MetaString& aUserName)
		: type(aType),
		  name(aName),
		  userName(aUserName)
	{
	}

	CallerName()
		: type(obj_type_MAX)
	{
	}

	CallerName(const CallerName& o)
		: type(o.type),
		  name(o.name),
		  userName(o.userName)
	{
	}

	void operator =(const CallerName& o)
	{
		if (&o != this)
		{
			type = o.type;
			name = o.name;
			userName = o.userName;
		}
	}

	int type;
	QualifiedName name;
	Firebird::MetaString userName;
};

typedef Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<SINT64, ULONG> > > ReplBlobMap;
typedef Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<SLONG, blb*> > > BlobUtilMap;

inline constexpr int DEFAULT_LOCK_TIMEOUT = -1; // infinite
inline constexpr const char* TRA_BLOB_SPACE = "fb_blob_";
inline constexpr const char* TRA_UNDO_SPACE = "fb_undo_";
inline constexpr ULONG MAX_TEMP_BLOBS = 1000;

class jrd_tra : public pool_alloc<type_tra>
{
	typedef Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<USHORT, SINT64> > > GenIdCache;

	static const size_t MAX_UNDO_RECORDS = 2;
	typedef Firebird::HalfStaticArray<Record*, MAX_UNDO_RECORDS> UndoRecordList;

public:
	enum wait_t {
		tra_no_wait,
		tra_probe,
		tra_wait
	};

	jrd_tra(MemoryPool* p, Firebird::MemoryStats* parent_stats,
			Attachment* attachment, jrd_tra* outer)
	:	tra_attachment(attachment),
		tra_pool(p),
		tra_memory_stats(parent_stats),
		tra_blobs_tree(*p),
		tra_blobs(outer ? outer->tra_blobs : &tra_blobs_tree),
		tra_fetched_blobs(*p),
		tra_repl_blobs(*p),
		tra_blob_util_map(*p),
		tra_arrays(NULL),
		tra_deferred_job(NULL),
		tra_resources(*p),
		tra_context_vars(*p),
		tra_lock_timeout(DEFAULT_LOCK_TIMEOUT),
		tra_timestamp(Firebird::TimeZoneUtil::getCurrentSystemTimeStamp()),
		tra_stats(*p),
		tra_open_cursors(*p),
		tra_outer(outer),
		tra_snapshot_handle(0),
		tra_snapshot_number(0),
		tra_sorts(*p, attachment->att_database),
		tra_gen_ids(NULL),
		tra_replicator(NULL),
		tra_interface(NULL),
		tra_blob_space(NULL),
		tra_undo_space(NULL),
		tra_undo_records(*p),
		tra_timezone_snapshot(NULL),
		tra_user_management(NULL),
		tra_sec_db_context(NULL),
		tra_mapping_list(NULL),
		tra_dbcreators_list(nullptr),
		tra_autonomous_pool(NULL),
		tra_autonomous_cnt(0)
	{
	}

	~jrd_tra();

	static jrd_tra* create(MemoryPool* pool, Attachment* attachment, jrd_tra* outer)
	{
		jrd_tra* const transaction =
			FB_NEW_POOL(*pool) jrd_tra(pool, &attachment->att_memory_stats, attachment, outer);

		if (!outer)
		{
			pool->setStatsGroup(transaction->tra_memory_stats);
		}

		return transaction;
	}

	static void destroy(Attachment* const attachment, jrd_tra* const transaction)
	{
		if (transaction)
		{
			if (!attachment)
				delete transaction;
			else if (transaction->tra_outer)
			{
				jrd_tra* outer = transaction->tra_outer;
				MemoryPool* const pool = transaction->tra_pool;
				delete transaction;
				outer->releaseAutonomousPool(pool);
			}
			else
			{
				MemoryPool* const pool = transaction->tra_pool;
				Firebird::MemoryStats temp_stats;
				pool->setStatsGroup(temp_stats);
				delete transaction;
				attachment->deletePool(pool);
			}
		}
	}

	Attachment* getAttachment() noexcept
	{
		return tra_attachment;
	}

	dsql_dbb* getDsqlAttachment() noexcept
	{
		return tra_attachment->att_dsql_instance;
	}

	JTransaction* getInterface(bool create);
	void setInterface(JTransaction* jt);

	FB_API_HANDLE tra_public_handle;	// Public handle
	Attachment* tra_attachment;			// database attachment
	TraNumber tra_number;				// transaction number
	TraNumber tra_top;					// highest transaction in snapshot
	TraNumber tra_oldest;				// oldest interesting transaction
	TraNumber tra_oldest_active;		// record versions older than this can be
										// gargage-collected by this tx
	TraNumber tra_att_oldest_active;	// oldest active transaction in the same attachment
	TraNumber tra_initial_number;		// initial transaction number, not changed by retain context
	jrd_tra* tra_next;					// next transaction in attachment
	MemoryPool* const tra_pool;			// pool for transaction
	Firebird::MemoryStats	tra_memory_stats;
	BlobIndexTree tra_blobs_tree;		// list of active blobs
	BlobIndexTree* tra_blobs;			// pointer to actual list of active blobs
	FetchedBlobIdTree tra_fetched_blobs;	// list of fetched blobs
	ReplBlobMap tra_repl_blobs;			// map of blob IDs replicated in this transaction
	BlobUtilMap tra_blob_util_map;		// map of blob IDs for RDB$BLOB_UTIL package
	ArrayField*	tra_arrays;				// Linked list of active arrays
	Lock*		tra_lock;				// lock for transaction
	Lock*		tra_alter_db_lock;		// lock for ALTER DATABASE statement(s)
	vec<Lock*>*			tra_relation_locks;	// locks for relations
	TransactionBitmap*	tra_commit_sub_trans;	// committed sub-transactions
	Savepoint*	tra_save_point;			// list of savepoints
	Savepoint*	tra_save_free;			// free savepoints
	SavNumber tra_save_point_number;	// next save point number to use
	ULONG tra_flags;
	DeferredJob*	tra_deferred_job;	// work deferred to commit time
	ResourceList tra_resources;			// resource existence list
	Firebird::StringMap tra_context_vars; // Context variables for the transaction
	traRpbList* tra_rpblist;			// active record_param's of given transaction
	UCHAR tra_use_count;				// use count for safe AST delivery
	UCHAR tra_callback_count;			// callback count for 'execute statement'
	SSHORT tra_lock_timeout;			// in seconds, -1 means infinite, 0 means NOWAIT
	ULONG tra_next_blob_id;     		// ID of the previous blob or array created in this transaction
	ULONG tra_temp_blobs_count;			// Number of active temporary blobs
	const ISC_TIMESTAMP_TZ tra_timestamp;	// transaction start time
	Request* tra_requests;				// Doubly linked list of requests active in this transaction
	MonitoringSnapshot* tra_mon_snapshot;	// Database state snapshot (for monitoring purposes)
	RuntimeStatistics tra_stats;
	Firebird::Array<DsqlCursor*> tra_open_cursors;
	bool tra_in_use;					// transaction in use (can't be committed or rolled back)
	jrd_tra* const tra_outer;			// outer transaction of an autonomous transaction
	CallerName tra_caller_name;			// caller object name
	SnapshotHandle tra_snapshot_handle;
	CommitNumber tra_snapshot_number;
	SortOwner tra_sorts;
	SLONG tra_blob_util_next = 1;

	EDS::Transaction *tra_ext_common;
	//Transaction *tra_ext_two_phase;
	GenIdCache* tra_gen_ids;
	Firebird::IReplicatedTransaction* tra_replicator;

private:
	JTransaction* tra_interface;
	TempSpace* tra_blob_space;	// temp blob storage
	TempSpace* tra_undo_space;	// undo log storage

	UndoRecordList tra_undo_records;	// temporary records used for the undo purposes
	TimeZoneSnapshot* tra_timezone_snapshot;
	UserManagement* tra_user_management;
	SecDbContext* tra_sec_db_context;
	MappingList* tra_mapping_list;
	DbCreatorsList* tra_dbcreators_list;
	MemoryPool* tra_autonomous_pool;
	USHORT tra_autonomous_cnt;
	static const USHORT TRA_AUTONOMOUS_PER_POOL = 64;

public:
	MemoryPool* getAutonomousPool();
	void releaseAutonomousPool(MemoryPool* toRelease);
	jrd_tra* getOuter();

	SSHORT getLockWait() const noexcept
	{
		return -tra_lock_timeout;
	}

	TempSpace* getBlobSpace()
	{
		if (tra_outer)
			return tra_outer->getBlobSpace();

		if (!tra_blob_space)
		{
			fb_assert(!tra_outer);
			tra_blob_space = FB_NEW_POOL(*tra_pool) TempSpace(*tra_pool, TRA_BLOB_SPACE);
		}

		return tra_blob_space;
	}

	TempSpace* getUndoSpace()
	{
		if (!tra_undo_space)
		{
			tra_undo_space = FB_NEW_POOL(*tra_pool) TempSpace(*tra_pool, TRA_UNDO_SPACE);
		}

		return tra_undo_space;
	}

	Record* getUndoRecord(const Format* format)
	{
		for (Record** iter = tra_undo_records.begin(); iter != tra_undo_records.end(); ++iter)
		{
			Record* const record = *iter;
			fb_assert(record);

			if (!record->isTempActive())
			{
				// initialize record for reuse
				record->reset(format);
				record->setTempActive();
				return record;
			}
		}

		Record* const record = FB_NEW_POOL(*tra_pool) Record(*tra_pool, format, true);
		tra_undo_records.add(record);

		return record;
	}

	void unlinkFromAttachment();
	void linkToAttachment(Attachment* attachment);
	static void tra_abort(const char* reason);

	TimeZoneSnapshot* getTimeZoneSnapshot(thread_db* tdbb);
	UserManagement* getUserManagement();
	SecDbContext* getSecDbContext();
	SecDbContext* setSecDbContext(Firebird::IAttachment* att, Firebird::ITransaction* tra);
	void eraseSecDbContext();
	MappingList* getMappingList();
	Record* findNextUndo(VerbAction* before_this, jrd_rel* relation, SINT64 number);
	void listStayingUndo(jrd_rel* relation, SINT64 number, RecordStack &staying);
	Savepoint* startSavepoint(bool root = false);
	void rollbackSavepoint(thread_db* tdbb, bool preserveLocks = false);
	void rollbackToSavepoint(thread_db* tdbb, SavNumber number);
	void releaseSavepoint(thread_db* tdbb);
	DbCreatorsList* getDbCreatorsList();
	void checkBlob(thread_db* tdbb, const bid* blob_id, jrd_fld* fld, bool punt);

	GenIdCache* getGenIdCache()
	{
		if (!tra_gen_ids)
			tra_gen_ids = FB_NEW_POOL(*tra_pool) GenIdCache(*tra_pool);

		return tra_gen_ids;
	}
};

// System transaction is always transaction 0.
inline constexpr TraNumber TRA_system_transaction = 0;

// Flag definitions for tra_flags.

inline constexpr ULONG TRA_system				= 0x1L;			// system transaction
inline constexpr ULONG TRA_prepared				= 0x2L;			// transaction is in limbo
inline constexpr ULONG TRA_reconnected			= 0x4L;			// reconnect in progress
inline constexpr ULONG TRA_degree3				= 0x8L;			// serializeable transaction
inline constexpr ULONG TRA_write				= 0x10L;		// transaction has written
inline constexpr ULONG TRA_readonly				= 0x20L;		// transaction is readonly
inline constexpr ULONG TRA_prepare2				= 0x40L;		// transaction has updated RDB$TRANSACTIONS
inline constexpr ULONG TRA_ignore_limbo			= 0x80L;		// ignore transactions in limbo
inline constexpr ULONG TRA_invalidated 			= 0x100L;		// transaction invalidated by failed write
inline constexpr ULONG TRA_deferred_meta 		= 0x200L;		// deferred meta work posted
inline constexpr ULONG TRA_read_committed		= 0x400L;		// can see latest committed records
inline constexpr ULONG TRA_autocommit			= 0x800L;		// autocommits all updates
inline constexpr ULONG TRA_perform_autocommit	= 0x1000L;		// indicates autocommit is necessary
inline constexpr ULONG TRA_rec_version			= 0x2000L;		// don't wait for uncommitted versions
inline constexpr ULONG TRA_restart_requests		= 0x4000L;		// restart all requests in attachment
inline constexpr ULONG TRA_no_auto_undo			= 0x8000L;		// don't start a savepoint in TRA_start
inline constexpr ULONG TRA_precommitted			= 0x10000L;		// transaction committed at startup
inline constexpr ULONG TRA_own_interface		= 0x20000L;		// tra_interface was created for internal needs
inline constexpr ULONG TRA_read_consistency		= 0x40000L; 	// ensure read consistency in this transaction
inline constexpr ULONG TRA_ex_restart			= 0x80000L; 	// Exception was raised to restart request
inline constexpr ULONG TRA_replicating			= 0x100000L;	// transaction is allowed to be replicated
inline constexpr ULONG TRA_no_blob_check		= 0x200000L;	// disable blob access checking
inline constexpr ULONG TRA_auto_release_temp_blobid = 0x400000L;// remove temp ids of materialized user blobs from tra_blobs

// flags derived from TPB, see also transaction_options() at tra.cpp
inline constexpr ULONG TRA_OPTIONS_MASK = (TRA_degree3 | TRA_readonly | TRA_ignore_limbo | TRA_read_committed |
	TRA_autocommit | TRA_rec_version | TRA_read_consistency | TRA_no_auto_undo | TRA_restart_requests | TRA_auto_release_temp_blobid);

inline constexpr int TRA_MASK				= 3;
//inline constexpr int TRA_BITS_PER_TRANS	= 2;
//inline constexpr int TRA_TRANS_PER_BYTE	= 4;
inline constexpr int TRA_SHIFT				= 2;

#define TRANS_SHIFT(number)	(((number) & TRA_MASK) << 1)
#define TRANS_OFFSET(number)	((number) >> TRA_SHIFT)

// Transaction cleanup. If a database is never quiescent, look
// for "dead" active transactions every so often at transaction
// startup

inline constexpr int TRA_ACTIVE_CLEANUP = 100;

// Transaction states.  The first four are states found
// in the transaction inventory page; the last two are
// returned internally

inline constexpr int tra_active			= 0;	// Transaction is active
inline constexpr int tra_limbo			= 1;
inline constexpr int tra_dead			= 2;
inline constexpr int tra_committed		= 3;
inline constexpr int tra_us				= 4;	// Transaction is us
inline constexpr int tra_precommitted	= 5;	// Transaction is precommitted

// Deferred work blocks are used by the meta data handler to keep track
// of work deferred to commit time.  This are usually used to perform
// meta data updates

enum dfw_t {
	dfw_null,
	dfw_create_relation,
	dfw_delete_relation,
	dfw_update_format,
	dfw_create_index,
	dfw_delete_index,
	dfw_compute_security,
	dfw_add_shadow,
	dfw_delete_shadow,
	dfw_delete_shadow_nodelete,
	dfw_modify_file,
	dfw_erase_file,
	dfw_create_field,
	dfw_delete_field,
	dfw_modify_field,
	dfw_delete_global,
	dfw_delete_rfr,
	dfw_post_event,
	dfw_create_trigger,
	dfw_delete_trigger,
	dfw_modify_trigger,
	//dfw_load_triggers,
	dfw_grant,
	dfw_revoke,
	dfw_scan_relation,
	dfw_create_expression_index,
	dfw_create_procedure,
	dfw_modify_procedure,
	dfw_delete_procedure,
	dfw_delete_prm,
	dfw_create_collation,
	dfw_delete_collation,
	dfw_delete_exception,
	//dfw_unlink_file,
	dfw_delete_generator,
	dfw_create_function,
	dfw_modify_function,
	dfw_delete_function,
	dfw_add_difference,
	dfw_delete_difference,
	dfw_begin_backup,
	dfw_end_backup,
	dfw_user_management,
	dfw_modify_package_header,
	dfw_drop_package_header,
	dfw_drop_package_body,
	dfw_check_not_null,
	dfw_store_view_context_type,
	dfw_set_generator,
	dfw_change_repl_state,

	// deferred works argument types
	dfw_arg_index_name,		// index name for dfw_delete_index, mandatory
	dfw_arg_partner_rel_id,	// partner relation id for dfw_delete_index if index is FK, optional
	dfw_arg_proc_name,		// procedure name for dfw_delete_prm, mandatory
	dfw_arg_force_computed,	// we need to drop dependencies from a field that WAS computed
	dfw_arg_check_blr,		// check if BLR is still compilable
	dfw_arg_rel_name,		// relation name of a trigger
	dfw_arg_trg_type,		// trigger type
	dfw_arg_new_name,		// new name
	dfw_arg_field_not_null,	// set domain to not nullable
	dfw_db_crypt,			// change database encryption status
	dfw_set_linger,			// set database linger
	dfw_clear_cache			// clear user mapping cache
};

} //namespace Jrd

#endif // JRD_TRA_H
