/*
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef DSQL_EXPR_NODES_H
#define DSQL_EXPR_NODES_H

#include <optional>
#include "firebird/impl/blr.h"
#include "../dsql/Nodes.h"
#include "../dsql/NodePrinter.h"
#include "../common/classes/init.h"
#include "../common/classes/TriState.h"
#include "../dsql/pass1_proto.h"

class SysFunction;

namespace Jrd {

class ItemInfo;
class DeclareVariableNode;
class SubQuery;
class RelationSourceNode;
class ValueListNode;


class ArithmeticNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_ARITHMETIC>
{
public:
	ArithmeticNode(MemoryPool& pool, UCHAR aBlrOp, bool aDialect1,
		ValueExprNode* aArg1 = NULL, ValueExprNode* aArg2 = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(arg1);
		holder.add(arg2);
	}

	virtual const char* getCompatDialectVerb()
	{
		switch (blrOp)
		{
			case blr_add:
				return "add";

			case blr_subtract:
				return "subtract";

			case blr_multiply:
				return "multiply";

			case blr_divide:
				return "divide";

			default:
				fb_assert(false);
				return NULL;
		}
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

	static dsc* add(thread_db* tdbb, const dsc* desc1, const dsc* desc2, impure_value* value,
		const UCHAR blrOp, bool dialect1, SCHAR nodScale, USHORT nodFlags);

private:
	static dsc* addDialect1(thread_db* tdbb, const dsc* desc1, const dsc* desc2, impure_value* value,
		const UCHAR blrOp, SCHAR nodScale, USHORT nodFlags);
	static dsc* addDialect3(thread_db* tdbb, const dsc* desc1, const dsc* desc2, impure_value* value,
		const UCHAR blrOp, SCHAR nodScale, USHORT nodFlags);

	dsc* multiplyDialect1(const dsc* desc, impure_value* value) const;
	dsc* multiplyDialect3(const dsc* desc, impure_value* value) const;
	dsc* divideDialect3(const dsc* desc, impure_value* value) const;

	static dsc* addDateTime(thread_db* tdbb, const dsc* desc, impure_value* value, UCHAR blrOp, bool dialect1);
	static dsc* addSqlDate(const dsc* desc, impure_value* value, UCHAR blrOp);
	static dsc* addSqlTime(thread_db* tdbb, const dsc* desc, impure_value* value, UCHAR blrOp);
	static dsc* addTimeStamp(thread_db* tdbb, const dsc* desc, impure_value* value, UCHAR blrOp, bool dialect1);

private:
	void makeDialect1(dsc* desc, dsc& desc1, dsc& desc2);
	void makeDialect3(dsc* desc, dsc& desc1, dsc& desc2);

public:
	static void getDescDialect1(thread_db* tdbb, dsc* desc, const dsc& desc1, const dsc& desc2, UCHAR blrOp,
		SCHAR* nodScale, USHORT* nodFlags);
	static void getDescDialect3(thread_db* tdbb, dsc* desc, const dsc& desc1, const dsc& desc2, UCHAR blrOp,
		SCHAR* nodScale, USHORT* nodFlags);

public:
	Firebird::string label;
	NestConst<ValueExprNode> arg1;
	NestConst<ValueExprNode> arg2;
	const UCHAR blrOp;
	bool dialect1;
};


class ArrayNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_ARRAY>
{
public:
	ArrayNode(MemoryPool& pool, FieldNode* aField);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	// This class is used only in the parser. It turns in a FieldNode in dsqlPass.

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, Request* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	NestConst<FieldNode> field;
};


class AtNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_AT>
{
public:
	AtNode(MemoryPool& pool, ValueExprNode* aDateTimeArg = NULL, ValueExprNode* aZoneArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(dateTimeArg);
		holder.add(zoneArg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> dateTimeArg;
	NestConst<ValueExprNode> zoneArg;
};


class BoolAsValueNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_BOOL_AS_VALUE>
{
public:
	explicit BoolAsValueNode(MemoryPool& pool, BoolExprNode* aBoolean = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(boolean);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
	}

	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<BoolExprNode> boolean;
};


class CastNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CAST>
{
public:
	explicit CastNode(MemoryPool& pool, ValueExprNode* aSource = NULL, dsql_fld* aDsqlField = NULL,
		const Firebird::string& aFormat = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(source);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	void setDsqlDesc(const dsc& desc)
	{
		dsqlDesc = desc;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

	static dsc* perform(thread_db* tdbb, impure_value* impure, dsc* value,
		const dsc* castDesc, const ItemInfo* itemInfo, const Firebird::string& format = nullptr);

public:
	MetaName dsqlAlias;
	dsql_fld* dsqlField;
	NestConst<ValueExprNode> source;
	NestConst<ItemInfo> itemInfo;
	Firebird::string format;
	dsc castDesc;
	bool artificial;
};


class CoalesceNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_COALESCE>
{
public:
	explicit CoalesceNode(MemoryPool& pool, ValueListNode* aArgs = NULL)
		: TypedNode<ValueExprNode, ExprNode::TYPE_COALESCE>(pool),
		  args(aArgs)
	{
		castDesc.clear();
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(args);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

	virtual bool possiblyUnknown() const
	{
		return true;
	}

	virtual bool ignoreNulls(const StreamList& /*streams*/) const
	{
		return false;
	}

public:
	NestConst<ValueListNode> args;
	dsc castDesc;
};


class CollateNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_COLLATE>
{
public:
	CollateNode(MemoryPool& pool, ValueExprNode* aArg, const QualifiedName& aCollation);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		if (dsql)
			holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	static ValueExprNode* pass1Collate(DsqlCompilerScratch* dsqlScratch, ValueExprNode* input,
		QualifiedName& collation);

	// This class is used only in the parser. It turns in a CastNode in dsqlPass.

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, Request* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

private:
	static void assignFieldDtypeFromDsc(dsql_fld* field, const dsc* desc);

public:
	NestConst<ValueExprNode> arg;
	QualifiedName collation;
};


class ConcatenateNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CONCATENATE>
{
public:
	explicit ConcatenateNode(MemoryPool& pool, ValueExprNode* aArg1 = NULL, ValueExprNode* aArg2 = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg1);
		holder.add(arg2);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> arg1;
	NestConst<ValueExprNode> arg2;
};


class CurrentDateNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_DATE>
{
public:
	explicit CurrentDateNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_DATE>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;
};


class CurrentTimeNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIME>
{
public:
	CurrentTimeNode(MemoryPool& pool, unsigned aPrecision)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIME>(pool),
		  precision(aPrecision)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	unsigned precision;
};


class CurrentTimeStampNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIMESTAMP>
{
public:
	CurrentTimeStampNode(MemoryPool& pool, unsigned aPrecision)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_TIMESTAMP>(pool),
		  precision(aPrecision)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	unsigned precision;
};


class CurrentRoleNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_ROLE>
{
public:
	explicit CurrentRoleNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_ROLE>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;
};


class CurrentSchemaNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_SCHEMA>
{
public:
	explicit CurrentSchemaNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_SCHEMA>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	Firebird::string internalPrint(NodePrinter& printer) const override;
	ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch) override;
	void setParameterName(dsql_par* parameter) const override;
	void genBlr(DsqlCompilerScratch* dsqlScratch) override;
	void make(DsqlCompilerScratch* dsqlScratch, dsc* desc) override;

	void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc) override;
	ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const override;
	ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb) override;
	dsc* execute(thread_db* tdbb, Request* request) const override;
};


class CurrentUserNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_USER>
{
public:
	explicit CurrentUserNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_CURRENT_USER>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;
};


class DecodeNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_DECODE>
{
public:
	explicit DecodeNode(MemoryPool& pool, ValueExprNode* aTest = NULL,
				ValueListNode* aConditions = NULL, ValueListNode* aValues = NULL)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DECODE>(pool),
		  label(pool),
		  test(aTest),
		  conditions(aConditions),
		  values(aValues)
	{
		label = "DECODE";
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(test);
		holder.add(conditions);
		holder.add(values);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool possiblyUnknown() const
	{
		return true;
	}

	virtual bool ignoreNulls(const StreamList& /*streams*/) const
	{
		return false;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	Firebird::string label;
	NestConst<ValueExprNode> test;
	NestConst<ValueListNode> conditions;
	NestConst<ValueListNode> values;
};


class DefaultNode : public DsqlNode<DefaultNode, ExprNode::TYPE_DEFAULT>
{
public:
	explicit DefaultNode(MemoryPool& pool, const QualifiedName& aRelationName,
		const MetaName& aFieldName);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);
	static ValueExprNode* createFromField(thread_db* tdbb, CompilerScratch* csb, StreamType* map, jrd_fld* fld);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;

	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);

public:
	const QualifiedName relationName;
	const MetaName fieldName;

private:
	jrd_fld* field;
};


class DerivedExprNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_EXPR>
{
public:
	explicit DerivedExprNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_EXPR>(pool),
		  arg(NULL),
		  internalStreamList(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	// This is a non-DSQL node.

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const
	{
		ValueExprNode::internalPrint(printer);

		NODE_PRINT(printer, arg);
		NODE_PRINT(printer, internalStreamList);
		NODE_PRINT(printer, cursorNumber);

		return "DerivedExprNode";
	}

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void collectStreams(SortedStreamList& streamList) const;

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const CompilerScratch* csb,
		StreamType currentStream, SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> arg;
	Firebird::Array<StreamType> internalStreamList;
	std::optional<USHORT> cursorNumber;
};


class DomainValidationNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_DOMAIN_VALIDATION>
{
public:
	explicit DomainValidationNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DOMAIN_VALIDATION>(pool)
	{
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
	}

	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	dsc domDesc;
};


class ExtractNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_EXTRACT>
{
public:
	ExtractNode(MemoryPool& pool, UCHAR aBlrSubOp, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	UCHAR blrSubOp;
	NestConst<ValueExprNode> arg;
};


class FieldNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_FIELD>
{
public:
	FieldNode(MemoryPool& pool, dsql_ctx* context = NULL, dsql_fld* field = NULL, ValueListNode* indices = NULL);
	FieldNode(MemoryPool& pool, StreamType stream, USHORT id, bool aById);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	ValueExprNode* internalDsqlPass(DsqlCompilerScratch* dsqlScratch, RecordSourceNode** list);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;

	void setDsqlDesc(const dsc& desc)
	{
		dsqlDesc = desc;
	}

	virtual bool deterministic() const override
	{
		return true;
	}

	virtual bool possiblyUnknown() const
	{
		return false;
	}

	virtual bool ignoreNulls(const StreamList& streams) const
	{
		return streams.exist(fieldStream);
	}

	virtual void collectStreams(SortedStreamList& streamList) const
	{
		if (!streamList.exist(fieldStream))
			streamList.add(fieldStream);
	}

	virtual bool unmappable(const MapNode* /*mapNode*/, StreamType /*shellStream*/) const
	{
		return true;
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const CompilerScratch* csb,
		StreamType currentStream, SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

private:
	static dsql_fld* resolveContext(DsqlCompilerScratch* dsqlScratch,
		const QualifiedName& qualifier, dsql_ctx* context);

public:
	QualifiedName dsqlQualifier;
	MetaName dsqlName;
	dsql_ctx* const dsqlContext;
	dsql_fld* const dsqlField;
	NestConst<ValueListNode> dsqlIndices;
	const Format* format;
	const StreamType fieldStream;
	std::optional<USHORT> cursorNumber;
	const USHORT fieldId;
	const bool byId;
	bool dsqlCursorField;
};


class GenIdNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_GEN_ID>
{
public:
	GenIdNode(MemoryPool& pool, bool aDialect1,
			  const QualifiedName& name,
			  ValueExprNode* aArg,
			  bool aImplicit, bool aIdentity);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool deterministic() const override
	{
		return false;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	GeneratorItem generator;
	NestConst<ValueExprNode> arg;
	SLONG step;
	const bool dialect1;

private:
	bool sysGen;
	const bool implicit;
	const bool identity;
};


class InternalInfoNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_INTERNAL_INFO>
{
public:
	struct InfoAttr
	{
		const char* alias;
		unsigned mask;
	};

	static const InfoAttr INFO_TYPE_ATTRIBUTES[MAX_INFO_TYPE];

	explicit InternalInfoNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> arg;
};


class LiteralNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_LITERAL>
{
public:
	explicit LiteralNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_LITERAL>(pool)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);
	static void genConstant(DsqlCompilerScratch* dsqlScratch, const dsc* desc, bool negateValue, USHORT numStringLength = 0);
	static void genNegZero(DsqlCompilerScratch* dsqlScratch, int prec);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

	bool getBoolean() const
	{
		fb_assert(litDesc.dsc_dtype == dtype_boolean);
		return *litDesc.dsc_address != '\0';
	}

	SLONG getSlong() const
	{
		fb_assert(litDesc.dsc_dtype == dtype_long);
		return *reinterpret_cast<SLONG*>(litDesc.dsc_address);
	}

	void fixMinSInt32(MemoryPool& pool);
	void fixMinSInt64(MemoryPool& pool);
	void fixMinSInt128(MemoryPool& pool);

public:
	NestConst<IntlString> dsqlStr;
	dsc litDesc;
	USHORT litNumStringLength = 0;
};


class DsqlAliasNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_ALIAS>
{
public:
	DsqlAliasNode(MemoryPool& pool, const MetaName& aName, ValueExprNode* aValue)
		: TypedNode<ValueExprNode, ExprNode::TYPE_ALIAS>(pool),
		  name(aName),
		  value(aValue),
		  implicitJoin(NULL)
	{
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(value);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	void setDsqlDesc(const dsc& desc)
	{
		dsqlDesc = desc;
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, Request* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	const MetaName name;
	NestConst<ValueExprNode> value;
	NestConst<ImplicitJoin> implicitJoin;
};


class DsqlMapNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_MAP>
{
public:
	DsqlMapNode(MemoryPool& pool, dsql_ctx* aContext, dsql_map* aMap);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;

	void setDsqlDesc(const dsc& desc)
	{
		dsqlDesc = desc;
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, Request* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	dsql_ctx* context;
	dsql_map* map;
	bool setNullable;
	bool clearNull;
};


class DerivedFieldNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_FIELD>
{
public:
	DerivedFieldNode(MemoryPool& pool, const MetaName& aName, USHORT aScope,
				ValueExprNode* aValue)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_FIELD>(pool),
		  name(aName),
		  value(aValue),
		  context(NULL),
		  scope(aScope)
	{
	}

	// Construct already processed node.
	DerivedFieldNode(MemoryPool& pool, dsql_ctx* aContext, ValueExprNode* aValue)
		: TypedNode<ValueExprNode, ExprNode::TYPE_DERIVED_FIELD>(pool),
		  value(aValue),
		  context(aContext),
		  scope(0)
	{
	}

	static void getContextNumbers(Firebird::SortedArray<USHORT>& contextNumbers, const DsqlContextStack& contextStack);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		if (dsql)
			holder.add(value);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	void setDsqlDesc(const dsc& desc)
	{
		dsqlDesc = desc;
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, Request* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	MetaName name;
	NestConst<ValueExprNode> value;
	dsql_ctx* context;
	USHORT scope;
};


class LocalTimeNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_LOCAL_TIME>
{
public:
	LocalTimeNode(MemoryPool& pool, unsigned aPrecision)
		: TypedNode<ValueExprNode, ExprNode::TYPE_LOCAL_TIME>(pool),
		  precision(aPrecision)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	unsigned precision;
};


class LocalTimeStampNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_LOCAL_TIMESTAMP>
{
public:
	LocalTimeStampNode(MemoryPool& pool, unsigned aPrecision)
		: TypedNode<ValueExprNode, ExprNode::TYPE_LOCAL_TIMESTAMP>(pool),
		  precision(aPrecision)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	unsigned precision;
};


class NegateNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_NEGATE>
{
public:
	explicit NegateNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> arg;
};


class NullNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_NULL>
{
private:
	friend class Firebird::GlobalPtr<NullNode>;

	explicit NullNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_NULL>(pool)
	{
		dsqlDesc.makeNullString();
	}

public:
	static NullNode* instance()
	{
		return &INSTANCE;
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual bool isSharedNode()
	{
		return true;
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

private:
	static Firebird::GlobalPtr<NullNode> INSTANCE;
};


class OrderNode : public DsqlNode<OrderNode, ExprNode::TYPE_ORDER>
{
public:
	enum NullsPlacement : UCHAR
	{
		NULLS_DEFAULT,
		NULLS_FIRST,
		NULLS_LAST
	};

	OrderNode(MemoryPool& pool, ValueExprNode* aValue);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		DsqlNode::getChildren(holder, dsql);
		holder.add(value);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual OrderNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;

public:
	NestConst<ValueExprNode> value;
	bool descending;
	NullsPlacement nullsPlacement;
};


class WindowClause : public DsqlNode<WindowClause, ExprNode::TYPE_WINDOW_CLAUSE>
{
public:
	// ListExprNode has no relation with this but works perfectly here for now.

	class Frame final : public TypedNode<ListExprNode, ExprNode::TYPE_WINDOW_CLAUSE_FRAME>
	{
	public:
		enum class Bound : UCHAR
		{
			// Warning: used in BLR
			PRECEDING = 0,
			FOLLOWING,
			CURRENT_ROW
		};

	public:
		explicit Frame(MemoryPool& pool, Bound aBound, ValueExprNode* aValue = NULL)
			: TypedNode(pool),
			  bound(aBound),
			  value(aValue)
		{
		}

	public:
		virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
		{
			ListExprNode::getChildren(holder, dsql);
			holder.add(value);
		}

		virtual Firebird::string internalPrint(NodePrinter& printer) const
		{
			NODE_PRINT_ENUM(printer, bound);
			NODE_PRINT(printer, value);

			return "WindowClause::Frame";
		}

		virtual Frame* dsqlPass(DsqlCompilerScratch* dsqlScratch)
		{
			Frame* node = FB_NEW_POOL(dsqlScratch->getPool()) Frame(dsqlScratch->getPool(), bound,
				doDsqlPass(dsqlScratch, value));

			if (node->value)
				node->value->setParameterType(dsqlScratch, [] (dsc* desc) { desc->makeLong(0); }, false);

			return node;
		}

		bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const
		{
			if (!ListExprNode::dsqlMatch(dsqlScratch, other, ignoreMapCast))
				return false;

			const Frame* o = nodeAs<Frame>(other);
			fb_assert(o);

			return bound == o->bound;
		}

		virtual Frame* dsqlFieldRemapper(FieldRemapper& visitor)
		{
			ListExprNode::dsqlFieldRemapper(visitor);
			return this;
		}

		virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
		virtual Frame* pass1(thread_db* tdbb, CompilerScratch* csb);
		virtual Frame* pass2(thread_db* tdbb, CompilerScratch* csb);
		virtual Frame* copy(thread_db* tdbb, NodeCopier& copier) const;

	public:
		Bound bound;
		NestConst<ValueExprNode> value;
	};

	class FrameExtent final : public TypedNode<ListExprNode, ExprNode::TYPE_WINDOW_CLAUSE_FRAME_EXTENT>
	{
	public:
		enum class Unit : UCHAR
		{
			// Warning: used in BLR
			RANGE = 0,
			ROWS
			//// TODO: SQL-2013: GROUPS
		};

	public:
		explicit FrameExtent(MemoryPool& pool, Unit aUnit, Frame* aFrame1 = NULL, Frame* aFrame2 = NULL)
			: TypedNode(pool),
			  unit(aUnit),
			  frame1(aFrame1),
			  frame2(aFrame2)
		{
		}

		static FrameExtent* createDefault(MemoryPool& p)
		{
			FrameExtent* frameExtent = FB_NEW_POOL(p) WindowClause::FrameExtent(p, Unit::RANGE);
			frameExtent->frame1 = FB_NEW_POOL(p) WindowClause::Frame(p, Frame::Bound::PRECEDING);
			frameExtent->frame2 = FB_NEW_POOL(p) WindowClause::Frame(p, Frame::Bound::CURRENT_ROW);
			return frameExtent;
		}

	public:
		virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
		{
			ListExprNode::getChildren(holder, dsql);
			holder.add(frame1);
			holder.add(frame2);
		}

		virtual Firebird::string internalPrint(NodePrinter& printer) const
		{
			NODE_PRINT_ENUM(printer, unit);
			NODE_PRINT(printer, frame1);
			NODE_PRINT(printer, frame2);

			return "WindowClause::FrameExtent";
		}

		virtual FrameExtent* dsqlPass(DsqlCompilerScratch* dsqlScratch);

		bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const
		{
			if (!ListExprNode::dsqlMatch(dsqlScratch, other, ignoreMapCast))
				return false;

			const FrameExtent* o = nodeAs<FrameExtent>(other);
			fb_assert(o);

			return unit == o->unit;
		}

		virtual FrameExtent* dsqlFieldRemapper(FieldRemapper& visitor)
		{
			ListExprNode::dsqlFieldRemapper(visitor);
			return this;
		}

		virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
		virtual FrameExtent* pass1(thread_db* tdbb, CompilerScratch* csb);
		virtual FrameExtent* pass2(thread_db* tdbb, CompilerScratch* csb);
		virtual FrameExtent* copy(thread_db* tdbb, NodeCopier& copier) const;

	public:
		Unit unit;
		NestConst<Frame> frame1;
		NestConst<Frame> frame2;
	};

	enum Exclusion : UCHAR
	{
		// Warning: used in BLR
		NO_OTHERS = 0,
		CURRENT_ROW,
		GROUP,
		TIES
	};

public:
	explicit WindowClause(MemoryPool& pool,
			const MetaName* aName = NULL,
			ValueListNode* aPartition = NULL,
			ValueListNode* aOrder = NULL,
			FrameExtent* aFrameExtent = NULL,
			Exclusion aExclusion = Exclusion::NO_OTHERS)
		: DsqlNode(pool),
		  name(aName),
		  partition(aPartition),
		  order(aOrder),
		  extent(aFrameExtent),
		  exclusion(aExclusion)
	{
	}

public:
	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(partition);
		holder.add(order);
		holder.add(extent);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const
	{
		NODE_PRINT(printer, partition);
		NODE_PRINT(printer, order);
		NODE_PRINT(printer, extent);
		NODE_PRINT_ENUM(printer, exclusion);

		return "WindowClause";
	}

	virtual WindowClause* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const
	{
		if (!DsqlNode::dsqlMatch(dsqlScratch, other, ignoreMapCast))
			return false;

		const WindowClause* o = nodeAs<WindowClause>(other);
		fb_assert(o);

		return exclusion == o->exclusion;
	}

	virtual WindowClause* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		DsqlNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual WindowClause* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		fb_assert(false);
		return this;
	}

	virtual WindowClause* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		fb_assert(false);
		return this;
	}

public:
	const MetaName* name;
	NestConst<ValueListNode> partition;
	NestConst<ValueListNode> order;
	NestConst<FrameExtent> extent;
	Exclusion exclusion;
};

// OVER is used only in DSQL. In the engine, normal aggregate functions are used in partitioned
// maps.
class OverNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_OVER>
{
public:
	explicit OverNode(MemoryPool& pool, AggNode* aAggExpr, const MetaName* aWindowName);
	explicit OverNode(MemoryPool& pool, AggNode* aAggExpr, WindowClause* aWindow);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		if (dsql)
		{
			holder.add(aggExpr);
			holder.add(window);
		}
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> aggExpr;
	const MetaName* windowName;
	NestConst<WindowClause> window;
};


class ParameterNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_PARAMETER>
{
private:
	// CVC: This is a guess for the length of the parameter for LIKE and others, when the
	// original dtype isn't string and force_varchar is true.
	static constexpr int LIKE_PARAM_LEN = 30;

public:
	explicit ParameterNode(MemoryPool& pool);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	void getChildren(NodeRefsHolder& holder, bool dsql) const override
	{
		ValueExprNode::getChildren(holder, dsql);

		if (!dsql)
			holder.add(argFlag);
	}

	Firebird::string internalPrint(NodePrinter& printer) const override;
	ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch) override;

	ParameterNode* dsqlFieldRemapper(FieldRemapper& visitor) override
	{
		ValueExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	void setParameterName(dsql_par* /*parameter*/) const override
	{
	}

	bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar) override;
	void genBlr(DsqlCompilerScratch* dsqlScratch) override;
	void make(DsqlCompilerScratch* dsqlScratch, dsc* desc) override;
	bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const override;

	Request* getParamRequest(Request* request) const;

	virtual bool deterministic() const override
	{
		return true;
	}

	void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc) override;
	ParameterNode* copy(thread_db* tdbb, NodeCopier& copier) const override;
	ParameterNode* pass1(thread_db* tdbb, CompilerScratch* csb) override;
	ParameterNode* pass2(thread_db* tdbb, CompilerScratch* csb) override;
	dsc* execute(thread_db* tdbb, Request* request) const override;

public:
	dsql_par* dsqlParameter = nullptr;
	NestConst<MessageNode> message;
	NestConst<ParameterNode> argFlag;
	NestConst<ItemInfo> argInfo;
	USHORT dsqlParameterIndex = 0;
	// This is an initial number as got from BLR.
	// Message can be modified during merge of SP/view subtrees
	USHORT messageNumber = MAX_USHORT;
	USHORT argNumber = 0;
	bool outerDecl = false;
};


class RecordKeyNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_RECORD_KEY>
{
public:
	RecordKeyNode(MemoryPool& pool, UCHAR aBlrOp, const QualifiedName& aDsqlQualifier = {});

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		if (dsql)
			holder.add(dsqlRelation);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool deterministic() const override
	{
		return true;
	}

	virtual bool possiblyUnknown() const
	{
		return false;
	}

	virtual bool ignoreNulls(const StreamList& streams) const
	{
		return streams.exist(recStream);
	}

	virtual void collectStreams(SortedStreamList& streamList) const
	{
		if (!streamList.exist(recStream))
			streamList.add(recStream);
	}

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const CompilerScratch* csb,
		StreamType currentStream, SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

	const char* getAlias(bool rdb) const
	{
		if (blrOp == blr_record_version2)
		{
			// ASF: It's on purpose that RDB$ prefix is always used here.
			// Absense of it with DB_KEY seems more a bug than feature.
			return RDB_RECORD_VERSION_NAME;
		}
		return (rdb ? RDB_DB_KEY_NAME : DB_KEY_NAME);
	}

private:
	static ValueExprNode* catenateNodes(thread_db* tdbb, ValueExprNodeStack& stack);

	void raiseError(dsql_ctx* context) const;

public:
	QualifiedName dsqlQualifier;
	NestConst<RecordSourceNode> dsqlRelation;
	StreamType recStream;
	const UCHAR blrOp;
	bool aggregate;
};


class ScalarNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_SCALAR>
{
public:
	explicit ScalarNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_SCALAR>(pool),
		  field(NULL),
		  subscripts(NULL)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	// This is a non-DSQL node.

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(field);
		holder.add(subscripts);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const
	{
		ValueExprNode::internalPrint(printer);

		NODE_PRINT(printer, field);
		NODE_PRINT(printer, subscripts);

		return "ScalarNode";
	}

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> field;
	NestConst<ValueListNode> subscripts;
};


class StmtExprNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_STMT_EXPR>
{
public:
	explicit StmtExprNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, ExprNode::TYPE_STMT_EXPR>(pool),
		  stmt(NULL),
		  expr(NULL)
	{
	}

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	// This is a non-DSQL node.

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		// Do not add the statement. We'll manually handle it in pass1 and pass2.
		holder.add(expr);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const
	{
		ValueExprNode::internalPrint(printer);

		NODE_PRINT(printer, stmt);
		NODE_PRINT(printer, expr);

		return "StmtExprNode";
	}

	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<StmtNode> stmt;
	NestConst<ValueExprNode> expr;
};


class StrCaseNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_STR_CASE>
{
public:
	StrCaseNode(MemoryPool& pool, UCHAR aBlrOp, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	const UCHAR blrOp;
	NestConst<ValueExprNode> arg;
};


class StrLenNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_STR_LEN>
{
public:
	StrLenNode(MemoryPool& pool, UCHAR aBlrSubOp, ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	UCHAR blrSubOp;
	NestConst<ValueExprNode> arg;
};


// This node is used for DSQL subqueries and for legacy (BLR-only) functionality.
class SubQueryNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_SUBQUERY>
{
public:
	explicit SubQueryNode(MemoryPool& pool, UCHAR aBlrOp, SelectExprNode* aDsqlSelectExpr = NULL,
		ValueExprNode* aValue1 = NULL, ValueExprNode* aValue2 = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const;

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual bool dsqlFieldFinder(FieldFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool unmappable(const MapNode* /*mapNode*/, StreamType /*shellStream*/) const
	{
		return false;
	}

	virtual bool possiblyUnknown() const
	{
		return true;
	}

	virtual bool ignoreNulls(const StreamList& /*streams*/) const
	{
		return false;
	}

	virtual void collectStreams(SortedStreamList& streamList) const;

	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value);

	virtual void findDependentFromStreams(const CompilerScratch* csb,
		StreamType currentStream, SortedStreamList* streamList);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<SelectExprNode> dsqlSelectExpr;
	NestConst<RseNode> rse;
	NestConst<ValueExprNode> value1;
	NestConst<ValueExprNode> value2;
	NestConst<SubQuery> subQuery;
	const UCHAR blrOp;
	bool ownSavepoint;
};


class SubstringNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_SUBSTRING>
{
public:
	explicit SubstringNode(MemoryPool& pool, ValueExprNode* aExpr = NULL,
		ValueExprNode* aStart = NULL, ValueExprNode* aLength = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(expr);
		holder.add(start);
		holder.add(length);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

	static dsc* perform(thread_db* tdbb, impure_value* impure, const dsc* valueDsc,
		const dsc* startDsc, const dsc* lengthDsc);

public:
	NestConst<ValueExprNode> expr;
	NestConst<ValueExprNode> start;
	NestConst<ValueExprNode> length;
};


class SubstringSimilarNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_SUBSTRING_SIMILAR>
{
public:
	explicit SubstringSimilarNode(MemoryPool& pool, ValueExprNode* aExpr = NULL,
		ValueExprNode* aPattern = NULL, ValueExprNode* aEscape = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(expr);
		holder.add(pattern);
		holder.add(escape);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<ValueExprNode> expr;
	NestConst<ValueExprNode> pattern;
	NestConst<ValueExprNode> escape;
};


class SysFuncCallNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_SYSFUNC_CALL>
{
public:
	explicit SysFuncCallNode(MemoryPool& pool, const MetaName& aName,
		ValueListNode* aArgs = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(args);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool deterministic() const override;

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	MetaName name;
	NestConst<ValueListNode> args;
	const SysFunction* function;
	bool dsqlSpecialSyntax;
};


class TrimNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_TRIM>
{
public:
	explicit TrimNode(MemoryPool& pool, UCHAR aWhere, UCHAR aWhat,
		ValueExprNode* aValue = NULL, ValueExprNode* aTrimChars = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(value);
		holder.add(trimChars);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	UCHAR where;
	UCHAR what;
	NestConst<ValueExprNode> value;
	NestConst<ValueExprNode> trimChars;	// may be NULL
};


class UdfCallNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_UDF_CALL>
{
private:
	struct Impure
	{
		impure_value value;	// must be first
		Firebird::Array<UCHAR>* temp;
	};

public:
	UdfCallNode(MemoryPool& pool, const QualifiedName& aName = {},
		ValueListNode* aArgs = nullptr, Firebird::ObjectsArray<MetaName>* aDsqlArgNames = nullptr);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

public:
	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(args);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool deterministic() const override;

	virtual bool possiblyUnknown() const
	{
		return true;
	}

	virtual bool ignoreNulls(const StreamList& /*streams*/) const
	{
		return false;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	QualifiedName name;
	NestConst<ValueListNode> args;
	NestConst<Firebird::ObjectsArray<MetaName>> dsqlArgNames;
	NestConst<Function> function;

private:
	dsql_udf* dsqlFunction = nullptr;
	bool isSubRoutine = false;
};


class ValueIfNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_VALUE_IF>
{
public:
	explicit ValueIfNode(MemoryPool& pool, BoolExprNode* aCondition = NULL,
		ValueExprNode* aTrueValue = NULL, ValueExprNode* aFalseValue = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);

		holder.add(condition);
		holder.add(trueValue);
		holder.add(falseValue);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual bool setParameterType(DsqlCompilerScratch* dsqlScratch,
		std::function<void (dsc*)> makeDesc, bool forceVarChar);
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);

	virtual bool possiblyUnknown() const
	{
		return true;
	}

	virtual bool ignoreNulls(const StreamList& /*streams*/) const
	{
		return false;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual bool sameAs(const ExprNode* other, bool ignoreStreams) const;
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	NestConst<BoolExprNode> condition;
	NestConst<ValueExprNode> trueValue;
	NestConst<ValueExprNode> falseValue;
	bool dsqlGenCast = true;
};


class VariableNode final : public TypedNode<ValueExprNode, ExprNode::TYPE_VARIABLE>
{
public:
	explicit VariableNode(MemoryPool& pool);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;

	void setDsqlDesc(const dsc& desc)
	{
		dsqlDesc = desc;
	}

	Request* getVarRequest(Request* request) const;

	virtual bool deterministic() const override
	{
		return false;
	}

	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual dsc* execute(thread_db* tdbb, Request* request) const;

public:
	MetaName dsqlName;
	NestConst<dsql_var> dsqlVar;
	NestConst<DeclareVariableNode> varDecl;
	NestConst<ItemInfo> varInfo;
	USHORT varId = 0;
	bool outerDecl = false;
};


} // namespace

#endif // DSQL_EXPR_NODES_H
