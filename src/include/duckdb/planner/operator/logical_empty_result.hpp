//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/planner/operator/logical_empty_result.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/planner/logical_operator.hpp"

namespace duckdb {

//! LogicalEmptyResult returns an empty result. This is created by the optimizer if it can reason that ceratin parts of
//! the tree will always return an empty result.
class LogicalEmptyResult : public LogicalOperator {
	LogicalEmptyResult();

public:
	explicit LogicalEmptyResult(unique_ptr<LogicalOperator> op);

	//! The set of return types of the empty result
	vector<LogicalType> return_types;
	//! The columns that would be bound at this location (if the subtree was not optimized away)
	vector<ColumnBinding> bindings;

public:
	vector<ColumnBinding> GetColumnBindings() override {
		return bindings;
	}
	void Serialize(FieldWriter &writer) const override;
	static unique_ptr<LogicalOperator> Deserialize(ClientContext &context, LogicalOperatorType type,
	                                               FieldReader &reader);

protected:
	void ResolveTypes() override {
		this->types = return_types;
	}
};
} // namespace duckdb
