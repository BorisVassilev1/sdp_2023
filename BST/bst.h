#pragma once
#include <cstddef>
#include <iostream>
#include <vector>


template<class T>
class BST {
public: 
	BST() : root(nullptr), size(0) {}	
	
	void insert(const T &newData) {
		Node **current = &this->root;
		
		while(*current != nullptr) {
			if((*current)->data < newData) {
				current = &(*current)->right;
			} else {
				current = &(*current)->left;
			}
		}
		*current = new Node{newData};
		++ size;
	}

	int height() {
		return heightHelper(this->root);
	}

	~BST() {
		deleteHelper(this->root);
	}

	void print() {
		printHelper(this->root);
	}

	void balance() {
		std::vector<Node *> sortedNodes;
		sortedNodes.reserve(this->size);
		getSortedNodes(this->root, sortedNodes);
		this->root = nullptr;
		buildTree(this->root, sortedNodes, 0, sortedNodes.size());
	}

private:
	struct Node {
		T data;
		Node *left;
		Node *right;
		Node(const T &data, Node *const left = nullptr, Node *const right = nullptr) : data(data), left(left), right(right) {}
	};
private:
	Node *root;
	std::size_t size;

	void deleteHelper(Node *const node) {
		if(node != nullptr) {
			deleteHelper(node->left);
			deleteHelper(node->right);
			delete node;
		}
	}

	void printHelper(Node *const node) {
		if(node == nullptr) return;
		printHelper(node->left);
		std::cout << node->data << ", ";
		printHelper(node->right);
	}
	
	int heightHelper(Node *const node) {
		if(node == nullptr) return 0;
		return std::max(heightHelper(node->left), heightHelper(node->right)) + 1;
	}

	void getSortedNodes(Node *const node, std::vector<Node *> &nodes) {
		if(node == nullptr) return;
		getSortedNodes(node->left, nodes);
		nodes.push_back(node);
		getSortedNodes(node->right, nodes);
		node->left = nullptr;
		node->right = nullptr;
	}

	void buildTree(Node *&current, std::vector<Node *> &nodes, std::size_t start, std::size_t end) {
		if(start >= end) return;
		std::size_t mid = start + (end - start) / 2;
		current = nodes[mid];
		buildTree(current->left, nodes, start, mid);
		buildTree(current->right, nodes, mid + 1, end);
	}
};
