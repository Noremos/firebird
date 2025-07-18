/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cmp.cpp
 *	DESCRIPTION:	Request compiler
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
 * 2001.07.28: John Bellardo: Added code to handle rse_skip.
 * 2001.07.17 Claudio Valderrama: Stop crash when parsing user-supplied SQL plan.
 * 2001.10.04 Claudio Valderrama: Fix annoying & invalid server complaint about
 *   triggers not having REFERENCES privilege over their owner table.
 * 2002.02.24 Claudio Valderrama: substring() should signal output as string even
 *   if source is blob and should check implementation limits on field lengths.
 * 2002.02.25 Claudio Valderrama: concatenate() should be a civilized function.
 *   This closes the heart of SF Bug #518282.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>				// abort
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/align.h"
#include "../jrd/lls.h"
#include "../jrd/exe.h"
#include "../jrd/scl.h"
#include "../jrd/tra.h"
#include "../jrd/lck.h"
#include "../jrd/irq.h"
#include "../jrd/drq.h"
#include "../jrd/intl.h"
#include "../jrd/btr.h"
#include "../jrd/sort.h"
#include "../common/gdsassert.h"
#include "../jrd/cmp_proto.h"
#include "../common/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/fun_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../common/dsc_proto.h"
#include "../jrd/optimizer/Optimizer.h"

#include "../jrd/DataTypeUtil.h"
#include "../jrd/SysFunction.h"

// Pick up relation ids
#include "../jrd/ini.h"

#include "../common/classes/auto.h"
#include "../common/utils_proto.h"
#include "../dsql/Nodes.h"
#include "../jrd/RecordSourceNodes.h"
#include "../jrd/recsrc/RecordSource.h"
#include "../jrd/recsrc/Cursor.h"
#include "../jrd/Function.h"
#include "../dsql/BoolNodes.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"

using namespace Jrd;
using namespace Firebird;


#ifdef CMP_DEBUG
#include <stdarg.h>
IMPLEMENT_TRACE_ROUTINE(cmp_trace, "CMP")
#endif


// Clone a node.
ValueExprNode* CMP_clone_node(thread_db* tdbb, CompilerScratch* csb, ValueExprNode* node)
{
	SubExprNodeCopier copier(csb->csb_pool, csb);
	return copier.copy(tdbb, node);
}


// Clone a value node for the optimizer.
// Make a copy of the node (if necessary) and assign impure space.

ValueExprNode* CMP_clone_node_opt(thread_db* tdbb, CompilerScratch* csb, ValueExprNode* node)
{
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	if (nodeIs<ParameterNode>(node))
		return node;

	SubExprNodeCopier copier(csb->csb_pool, csb);
	ValueExprNode* clone = copier.copy(tdbb, node);
	ExprNode::doPass2(tdbb, csb, &clone);

	return clone;
}

BoolExprNode* CMP_clone_node_opt(thread_db* tdbb, CompilerScratch* csb, BoolExprNode* node)
{
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	SubExprNodeCopier copier(csb->csb_pool, csb);
	BoolExprNode* clone = copier.copy(tdbb, node);
	ExprNode::doPass2(tdbb, csb, &clone);

	return clone;
}

// Compile a statement.
Statement* CMP_compile(thread_db* tdbb, const UCHAR* blr, ULONG blrLength, bool internalFlag,
	ULONG dbginfoLength, const UCHAR* dbginfo)
{
	Statement* statement = nullptr;

	SET_TDBB(tdbb);
	const auto att = tdbb->getAttachment();

	// 26.09.2002 Nickolay Samofatov: default memory pool will become statement pool
	// and will be freed by CMP_release
	const auto newPool = att->createPool();

	try
	{
		Jrd::ContextPoolHolder context(tdbb, newPool);

		const auto csb = PAR_parse(tdbb, blr, blrLength, internalFlag, dbginfoLength, dbginfo);

		statement = Statement::makeStatement(tdbb, csb, internalFlag);

#ifdef CMP_DEBUG
		if (csb->csb_dump.hasData())
		{
			csb->dump("streams:\n");
			for (StreamType i = 0; i < csb->csb_n_stream; ++i)
			{
				const CompilerScratch::csb_repeat& s = csb->csb_rpt[i];
				csb->dump(
					"\t%2d - view_stream: %2d; alias: %s; relation: %s; procedure: %s; view: %s\n",
					i, s.csb_view_stream,
					(s.csb_alias ? s.csb_alias->c_str() : ""),
					(s.csb_relation ? s.csb_relation->rel_name.c_str() : ""),
					(s.csb_procedure ? s.csb_procedure->getName().toString().c_str() : ""),
					(s.csb_view ? s.csb_view->rel_name.c_str() : ""));
			}

			cmp_trace("\n%s\n", csb->csb_dump.c_str());
		}
#endif

		statement->verifyAccess(tdbb);

		delete csb;
	}
	catch (const Firebird::Exception& ex)
	{
		ex.stuffException(tdbb->tdbb_status_vector);
		if (statement)
			statement->release(tdbb);
		else
			att->deletePool(newPool);
		ERR_punt();
	}

	return statement;
}

Request* CMP_compile_request(thread_db* tdbb, const UCHAR* blr, ULONG blrLength, bool internalFlag)
{
/**************************************
 *
 *	C M P _ c o m p i l e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Compile a BLR request.
 *
 **************************************/
	SET_TDBB(tdbb);

	auto statement = CMP_compile(tdbb, blr, blrLength, internalFlag, 0, nullptr);
	auto request = statement->getRequest(tdbb, 0);

	return request;
}


CompilerScratch::csb_repeat* CMP_csb_element(CompilerScratch* csb, StreamType element)
{
/**************************************
 *
 *	C M P _ c s b _ e l e m e n t
 *
 **************************************
 *
 * Functional description
 *	Find tail element of compiler scratch block.  If the csb isn't big
 *	enough, extend it.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	CompilerScratch::csb_repeat empty_item;
	while (element >= csb->csb_rpt.getCount()) {
		csb->csb_rpt.add(empty_item);
	}
	return &csb->csb_rpt[element];
}


const Format* CMP_format(thread_db* tdbb, CompilerScratch* csb, StreamType stream)
{
/**************************************
 *
 *	C M P _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Pick up a format for a stream.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	CompilerScratch::csb_repeat* const tail = &csb->csb_rpt[stream];

	if (!tail->csb_format)
	{
		if (tail->csb_relation)
			tail->csb_format = MET_current(tdbb, tail->csb_relation);
		else if (tail->csb_procedure)
			tail->csb_format = tail->csb_procedure->prc_record_format;
		else if (tail->csb_table_value_fun)
			tail->csb_format = tail->csb_table_value_fun->recordFormat;
		//// TODO: LocalTableSourceNode
		else
			IBERROR(222);	// msg 222 bad blr - invalid stream
	}

	fb_assert(tail->csb_format);
	return tail->csb_format;
}


IndexLock* CMP_get_index_lock(thread_db* tdbb, jrd_rel* relation, USHORT id)
{
/**************************************
 *
 *	C M P _ g e t _ i n d e x _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Get index lock block for index.  If one doesn't exist,
 *	make one.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	DEV_BLKCHK(relation, type_rel);

	if (relation->rel_id < (USHORT) rel_MAX) {
		return NULL;
	}

	// for to find an existing block

	for (IndexLock* index = relation->rel_index_locks; index; index = index->idl_next)
	{
		if (index->idl_id == id) {
			return index;
		}
	}

	IndexLock* index = FB_NEW_POOL(*relation->rel_pool) IndexLock();
	index->idl_next = relation->rel_index_locks;
	relation->rel_index_locks = index;
	index->idl_relation = relation;
	index->idl_id = id;
	index->idl_count = 0;

	Lock* lock = FB_NEW_RPT(*relation->rel_pool, 0) Lock(tdbb, sizeof(SLONG), LCK_idx_exist);
	index->idl_lock = lock;
	lock->setKey((relation->rel_id << 16) | id);

	return index;
}


void CMP_post_access(thread_db* tdbb,
					 CompilerScratch* csb,
					 const MetaName& security_name,
					 SLONG ssRelationId,			// SQL SECURITY relation in which context permissions should be check
					 SecurityClass::flags_t mask,
					 ObjectType obj_type,
					 const QualifiedName& name,
					 const MetaName& columnName)
{
/**************************************
 *
 *	C M P _ p o s t _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *	Post access to security class to request.
 *      We append the new security class to the existing list of
 *      security classes for that request.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(view, type_rel);

	// allow all access to internal requests

	if (csb->csb_g_flags & (csb_internal | csb_ignore_perm))
		return;

	SET_TDBB(tdbb);

	AccessItem access(security_name, ssRelationId, name, obj_type, mask, columnName);

	FB_SIZE_T i;

	if (!csb->csb_access.find(access, i))
		csb->csb_access.insert(i, access);
}


void CMP_post_resource(	ResourceList* rsc_ptr, void* obj, Resource::rsc_s type, USHORT id)
{
/**************************************
 *
 *	C M P _ p o s t _ r e s o u r c e
 *
 **************************************
 *
 * Functional description
 *	Post a resource usage to the compiler scratch block.
 *
 **************************************/
	// Initialize resource block
	Resource resource(type, id, NULL, NULL, NULL);
	switch (type)
	{
	case Resource::rsc_relation:
	case Resource::rsc_index:
		resource.rsc_rel = (jrd_rel*) obj;
		break;
	case Resource::rsc_procedure:
	case Resource::rsc_function:
		resource.rsc_routine = (Routine*) obj;
		break;
	case Resource::rsc_collation:
		resource.rsc_coll = (Collation*) obj;
		break;
	default:
		BUGCHECK(220);			// msg 220 unknown resource
		break;
	}

	// Add it into list if not present already
	FB_SIZE_T pos;
	if (!rsc_ptr->find(resource, pos))
		rsc_ptr->insert(pos, resource);
}


void CMP_release(thread_db* tdbb, Request* request)
{
/**************************************
 *
 *	C M P _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release a request's statement.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);
	request->getStatement()->release(tdbb);
}


StreamType* CMP_alloc_map(thread_db* tdbb, CompilerScratch* csb, StreamType stream)
{
/**************************************
 *
 *	C M P _ a l l o c _ m a p
 *
 **************************************
 *
 * Functional description
 *	Allocate and initialize stream map for view processing.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);

	SET_TDBB(tdbb);

	fb_assert(stream <= MAX_STREAMS);
	StreamType* const p = FB_NEW_POOL(*tdbb->getDefaultPool()) StreamType[STREAM_MAP_LENGTH];
	memset(p, 0, sizeof(StreamType[STREAM_MAP_LENGTH]));
	p[0] = stream;
	csb->csb_rpt[stream].csb_map = p;

	return p;
}


USHORT NodeCopier::getFieldId(const FieldNode* field)
{
	return field->fieldId;
}


// Copy items' information into appropriate node.
ItemInfo* CMP_pass2_validation(thread_db* tdbb, CompilerScratch* csb, const Item& item)
{
	ItemInfo itemInfo;
	return csb->csb_map_item_info.get(item, itemInfo) ?
		FB_NEW_POOL(*tdbb->getDefaultPool()) ItemInfo(*tdbb->getDefaultPool(), itemInfo) : NULL;
}


bool CMP_procedure_arguments(
	thread_db* tdbb,
	CompilerScratch* csb,
	Routine* routine,
	bool isInput,
	USHORT argCount,
	ObjectsArray<MetaName>* argNames,
	NestConst<ValueListNode>& sources,
	NestConst<ValueListNode>& targets,
	NestConst<MessageNode>& message,
	Arg::StatusVector& mismatchStatus)
{
	auto& pool = *tdbb->getDefaultPool();
	auto& fields = isInput ? routine->getInputFields() : routine->getOutputFields();
	const auto format = isInput ? routine->getInputFormat() : routine->getOutputFormat();

	if ((isInput && fields.hasData() && (argCount || routine->getDefaultCount())) ||
		(!isInput && argCount))
	{
		if (isInput)
			sources->items.resize(fields.getCount());
		else
			targets = FB_NEW_POOL(pool) ValueListNode(pool, argCount);

		// We have a few parameters. Get on with creating the message block
		// Outer messages map may start with 2, but they are always in the routine start.
		USHORT n = ++csb->csb_msg_number;
		if (n < 2)
			csb->csb_msg_number = n = 2;
		const auto tail = CMP_csb_element(csb, n);

		/* dimitr: procedure (with its parameter formats) is allocated out of
					its own pool (prc_request->req_pool) and can be freed during
					the cache cleanup (MET_clear_cache). Since the current
					tdbb default pool is different from the procedure's one,
					it's dangerous to copy a pointer from one request to another.
					As an experiment, I've decided to copy format by value
					instead of copying the reference. Since Format structure
					doesn't contain any pointers, it should be safe to use a
					default assignment operator which does a simple byte copy.
					This change fixes one serious bug in the current codebase.
					I think that this situation can (and probably should) be
					handled by the metadata cache (via incrementing prc_use_count)
					to avoid unexpected cache cleanups, but that area is out of my
					knowledge. So this fix should be considered a temporary solution.

		message->format = format;
		*/
		message = tail->csb_message = FB_NEW_POOL(pool) MessageNode(pool, *format);
		// --- end of fix ---
		message->messageNumber = n;

		const auto positionalArgCount = argNames ? argCount - argNames->getCount() : argCount;
		auto sourceArgIt = sources->items.begin();
		LeftPooledMap<MetaName, NestConst<ValueExprNode>> argsByName;

		if (positionalArgCount)
		{
			if (argCount > fields.getCount())
				mismatchStatus << Arg::Gds(isc_wronumarg);

			for (auto pos = 0u; pos < positionalArgCount; ++pos)
			{
				if (pos < fields.getCount())
				{
					const auto& parameter = fields[pos];

					if (argsByName.put(parameter->prm_name, *sourceArgIt))
						mismatchStatus << Arg::Gds(isc_param_multiple_assignments) << parameter->prm_name;
				}

				++sourceArgIt;
			}
		}

		if (argNames)
		{
			for (const auto& argName : *argNames)
			{
				if (argsByName.put(argName, *sourceArgIt++))
					mismatchStatus << Arg::Gds(isc_param_multiple_assignments) << argName;
			}
		}

		sourceArgIt = sources->items.begin();
		auto targetArgIt = targets->items.begin();

		for (auto& parameter : fields)
		{
			const auto argValue = argsByName.get(parameter->prm_name);
			const bool argExists = argsByName.exist(parameter->prm_name);

			if (argValue)
			{
				*sourceArgIt = *argValue;
				argsByName.remove(parameter->prm_name);
			}

			if (!argValue || !*argValue)
			{
				if (isInput)
				{
					if (parameter->prm_default_value)
						*sourceArgIt = CMP_clone_node(tdbb, csb, parameter->prm_default_value);
					else if (argExists)	// explicit DEFAULT in caller
					{
						FieldInfo fieldInfo;

						if (parameter->prm_mechanism != prm_mech_type_of &&
							!fb_utils::implicit_domain(parameter->prm_field_source.object.c_str()))
						{
							const QualifiedNameMetaNamePair entry(parameter->prm_field_source, {});

							if (!csb->csb_map_field_info.get(entry, fieldInfo))
							{
								dsc dummyDesc;
								MET_get_domain(tdbb, csb->csb_pool, parameter->prm_field_source, &dummyDesc, &fieldInfo);
								csb->csb_map_field_info.put(entry, fieldInfo);
							}
						}

						if (fieldInfo.defaultValue)
							*sourceArgIt = CMP_clone_node(tdbb, csb, fieldInfo.defaultValue);
						else
							*sourceArgIt = NullNode::instance();
					}
					else
						mismatchStatus << Arg::Gds(isc_param_no_default_not_specified) << parameter->prm_name;
				}
				else
					continue;
			}

			++sourceArgIt;

			const auto paramNode = FB_NEW_POOL(csb->csb_pool) ParameterNode(csb->csb_pool);
			paramNode->messageNumber = message->messageNumber;
			paramNode->message = message;
			paramNode->argNumber = parameter->prm_number * 2;

			const auto paramFlagNode = FB_NEW_POOL(csb->csb_pool) ParameterNode(csb->csb_pool);
			paramFlagNode->messageNumber = message->messageNumber;
			paramFlagNode->message = message;
			paramFlagNode->argNumber = parameter->prm_number * 2 + 1;

			paramNode->argFlag = paramFlagNode;

			*targetArgIt++ = paramNode;
		}

		if (argsByName.hasData())
		{
			for (const auto& argPair : argsByName)
				mismatchStatus << Arg::Gds(isc_param_not_exist) << argPair.first;
		}
	}
	else if (isInput && !argNames)
	{
		if (argCount > fields.getCount())
			mismatchStatus << Arg::Gds(isc_wronumarg);

		for (unsigned i = 0; i < fields.getCount(); ++i)
		{
			// default value for parameter
			if (i >= argCount)
			{
				auto parameter = fields[i];

				if (!parameter->prm_default_value)
					mismatchStatus << Arg::Gds(isc_param_no_default_not_specified) << parameter->prm_name;
			}
		}
	}

	return mismatchStatus.isEmpty();
}


void CMP_post_procedure_access(thread_db* tdbb, CompilerScratch* csb, jrd_prc* procedure)
{
/**************************************
 *
 *	C M P _ p o s t _ p r o c e d u r e _ a c c e s s
 *
 **************************************
 *
 * Functional description
 *
 *	The request will inherit access requirements to all the objects
 *	the called stored procedure has access requirements for.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(csb, type_csb);

	// allow all access to internal requests

	if (csb->csb_g_flags & (csb_internal | csb_ignore_perm))
		return;

	const SLONG ssRelationId = csb->csb_view ? csb->csb_view->rel_id : 0;

	CMP_post_access(tdbb, csb, procedure->getSecurityName().schema, ssRelationId,
		SCL_usage, obj_schemas, QualifiedName(procedure->getName().schema));

	// this request must have EXECUTE permission on the stored procedure
	if (procedure->getName().package.isEmpty())
	{
		CMP_post_access(tdbb, csb, procedure->getSecurityName().object, ssRelationId,
			SCL_execute, obj_procedures, procedure->getName());
	}
	else
	{
		CMP_post_access(tdbb, csb, procedure->getSecurityName().object, ssRelationId,
			SCL_execute, obj_packages, procedure->getName().getSchemaAndPackage());
	}

	// Add the procedure to list of external objects accessed
	ExternalAccess temp(ExternalAccess::exa_procedure, procedure->getId());
	FB_SIZE_T idx;
	if (!csb->csb_external.find(temp, idx))
		csb->csb_external.insert(idx, temp);
}


RecordSource* CMP_post_rse(thread_db* tdbb, CompilerScratch* csb, RseNode* rse)
{
/**************************************
 *
 *	C M P _ p o s t _ r s e
 *
 **************************************
 *
 * Functional description
 *	Perform actual optimization of an RseNode and clear activity.
 *
 **************************************/
	SET_TDBB(tdbb);

	fb_assert(csb->csb_currentCursorId);

	const auto rsb = Optimizer::compile(tdbb, csb, rse);

	// Mark all the substreams as inactive

	StreamList streams;
	rse->computeRseStreams(streams);

	for (const auto stream : streams)
		csb->csb_rpt[stream].deactivate();

	return rsb;
}
