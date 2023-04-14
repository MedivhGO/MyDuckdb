//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/execution/index/art/node256.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/execution/index/art/art.hpp"
#include "duckdb/execution/index/art/art_node.hpp"
#include "duckdb/execution/index/art/prefix.hpp"
#include "duckdb/execution/index/art/fixed_size_allocator.hpp"

namespace duckdb {

//! Node256 holds up to 256 ARTNode children which can be directly indexed by the key byte
class Node256 {
public:
	//! Number of non-null children
	uint16_t count;
	//! Compressed path (prefix)
	Prefix prefix;
	//! ART node pointers to the child nodes
	ARTNode children[ARTNode::NODE_256_CAPACITY];

public:
	//! Get a new pointer to a node, might cause a new buffer allocation, and initialize it
	static Node256 *New(ART &art, ARTNode &node);
	//! Free the node (and its subtree)
	static void Free(ART &art, ARTNode &node);
	//! Get a pointer to the node
	static inline Node256 *Get(const ART &art, const ARTNode ptr) {
		return art.n256_nodes->Get<Node256>(ptr);
	}
	//! Initializes all the fields of the node while growing a Node48 to a Node256
	static Node256 *GrowNode48(ART &art, ARTNode &node256, ARTNode &node48);

	//! Initializes a merge by incrementing the buffer IDs of the node
	void InitializeMerge(ART &art, const ARTFlags &flags);

	//! Insert a child node at byte
	static void InsertChild(ART &art, ARTNode &node, const uint8_t byte, const ARTNode child);
	//! Delete the child node at the respective byte
	static void DeleteChild(ART &art, ARTNode &node, const uint8_t byte);

	//! Replace the child node at the respective byte
	inline void ReplaceChild(const uint8_t byte, const ARTNode child) {
		children[byte] = child;
	}

	//! Get the child for the respective byte in the node
	inline ARTNode *GetChild(const uint8_t byte) {
		if (children[byte].IsSet()) {
			return &children[byte];
		}
		return nullptr;
	}
	//! Get the first child that is greater or equal to the specific byte
	ARTNode *GetNextChild(uint8_t &byte);

	//! Serialize an ART node
	BlockPointer Serialize(ART &art, MetaBlockWriter &writer);
	//! Deserialize this node
	void Deserialize(ART &art, MetaBlockReader &reader);

	//! Vacuum the children of the node
	void Vacuum(ART &art, const ARTFlags &flags);
};
} // namespace duckdb
