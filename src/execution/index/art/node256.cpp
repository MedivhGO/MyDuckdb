#include "duckdb/execution/index/art/node256.hpp"

#include "duckdb/execution/index/art/art.hpp"
#include "duckdb/execution/index/art/art_node.hpp"
#include "duckdb/execution/index/art/node48.hpp"
#include "duckdb/storage/meta_block_writer.hpp"
#include "duckdb/storage/meta_block_reader.hpp"

namespace duckdb {

Node256 *Node256::New(ART &art, ARTNode &node) {

	node.SetPtr(art.n256_nodes->New());
	node.type = (uint8_t)ARTNodeType::NODE_256;
	auto n256 = Node256::Get(art, node);

	n256->count = 0;
	n256->prefix.Initialize();

	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		n256->children[i].Reset();
	}

	return n256;
}

void Node256::Free(ART &art, ARTNode &node) {

	D_ASSERT(node.IsSet());
	D_ASSERT(!node.IsSwizzled());

	auto n256 = Node256::Get(art, node);

	if (!n256->count) {
		return;
	}

	// free all children
	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		if (n256->children[i].IsSet()) {
			ARTNode::Free(art, n256->children[i]);
		}
	}
}

Node256 *Node256::GrowNode48(ART &art, ARTNode &node256, ARTNode &node48) {

	auto n48 = Node48::Get(art, node48);
	auto n256 = Node256::New(art, node256);

	n256->count = n48->count;
	n256->prefix.Move(n48->prefix);

	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		if (n48->child_index[i] != ARTNode::EMPTY_MARKER) {
			n256->children[i] = n48->children[n48->child_index[i]];
		} else {
			n256->children[i].Reset();
		}
	}

	n48->count = 0;
	ARTNode::Free(art, node48);
	return n256;
}

void Node256::InitializeMerge(ART &art, const ARTFlags &flags) {

	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		if (children[i].IsSet()) {
			children[i].InitializeMerge(art, flags);
		}
	}
}

void Node256::InsertChild(ART &art, ARTNode &node, const uint8_t byte, const ARTNode child) {

	D_ASSERT(node.IsSet());
	D_ASSERT(!node.IsSwizzled());
	auto n256 = Node256::Get(art, node);

	// ensure that there is no other child at the same byte
	D_ASSERT(!n256->children[byte].IsSet());

	n256->count++;
	n256->children[byte] = child;
}

void Node256::DeleteChild(ART &art, ARTNode &node, const uint8_t byte) {

	D_ASSERT(node.IsSet());
	D_ASSERT(!node.IsSwizzled());
	auto n256 = Node256::Get(art, node);

	// free the child and decrease the count
	ARTNode::Free(art, n256->children[byte]);
	n256->count--;

	// shrink node to Node48
	if (n256->count <= ARTNode::NODE_256_SHRINK_THRESHOLD) {
		auto node256 = node;
		Node48::ShrinkNode256(art, node, node256);
	}
}

ARTNode *Node256::GetNextChild(uint8_t &byte) {

	for (idx_t i = byte; i < ARTNode::NODE_256_CAPACITY; i++) {
		if (children[i].IsSet()) {
			byte = i;
			return &children[i];
		}
	}
	return nullptr;
}

BlockPointer Node256::Serialize(ART &art, MetaBlockWriter &writer) {

	// recurse into children and retrieve child block pointers
	vector<BlockPointer> child_block_pointers;
	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		child_block_pointers.push_back(children[i].Serialize(art, writer));
	}

	// get pointer and write fields
	auto block_pointer = writer.GetBlockPointer();
	writer.Write(ARTNodeType::NODE_256);
	writer.Write<uint16_t>(count);
	prefix.Serialize(art, writer);

	// write child block pointers
	for (auto &child_block_pointer : child_block_pointers) {
		writer.Write(child_block_pointer.block_id);
		writer.Write(child_block_pointer.offset);
	}

	return block_pointer;
}

void Node256::Deserialize(ART &art, MetaBlockReader &reader) {

	count = reader.Read<uint16_t>();
	prefix.Deserialize(art, reader);

	// read child block pointers
	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		children[i] = ARTNode(reader);
	}
}

void Node256::Vacuum(ART &art, const ARTFlags &flags) {

	for (idx_t i = 0; i < ARTNode::NODE_256_CAPACITY; i++) {
		if (children[i].IsSet()) {
			ARTNode::Vacuum(art, children[i], flags);
		}
	}
}

} // namespace duckdb
