#include <bits/ranges_base.h>
#include <bits/utility.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <stack>
#include <vector>

struct Point {
	int x, y;
	Point(int x, int y) : x(x), y(y) {}

	Point operator+(const Point &other) { return Point(x + other.x, y + other.y); }
	bool  operator==(const Point &other) const { return x == other.x && y == other.y; }
};

std::ostream &operator<<(std::ostream &out, const Point &p) {
	return out << "(" << p.x << " " << p.y << ")" << std::endl;
}

template <std::size_t size>
struct Lab {
	int arr[size][size];

	bool checkBounds(size_t y, size_t x) { return y < size && x < size; }

	bool checkBounds(const Point &point) { return checkBounds(point.y, point.x); }

	int *operator[](int i) { return this->arr[i]; }

	int &operator[](const Point &point) { return this->arr[point.y][point.x]; }

	void printLine(std::size_t p) const {
		// compile time for loop
		[this, p]<std::size_t... q>(std::index_sequence<q...>) {
			((std::cout << arr[p][q] << " "), ...);
		}(std::make_index_sequence<size>{});
	}

	void print() const {
		[this]<size_t... p>(std::index_sequence<p...>) {
			((printLine(p), std::cout << std::endl), ...);
		}(std::make_index_sequence<size>{});
	}
};

struct Node {
	std::vector<Node *> children;
	Node			   *parent;
	Point				position;
	Node(const Point &position) : children(), parent(nullptr), position(position) {}
	bool isLeaf() {return children.empty(); }
};

Point dirs[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

template <size_t size>
Node *treeHelper(Lab<size> &lab, Point current, Point end) {
	Node *res = new Node(current);
	if (current == end) return res;
	lab[current] = 1;
	for (const auto &dir : dirs) {
		auto nextPos = current + dir;
		if (lab.checkBounds(nextPos) && !lab[nextPos]) {
			Node *child	  = treeHelper(lab, current + dir, end);
			if(child) {
				child->parent = res;
				res->children.push_back(child);
			}
		}
	}
	lab[current] = 0;
	if(res->children.empty()) {
		delete res;
		return nullptr;
	}
	return res;
}

template <size_t size>
Node *tree(const Lab<size> &lab, Point start, Point end) {
	lab.print();
	Lab<size>				   localLab = lab;
	std::stack<Node *> s;
	Node			  *root = treeHelper(localLab, start, end);

	return root;
}

void printTree(Node *root) {
	std::cout << root->position << "{" << std::endl;
	for(Node * child : root->children) {
		printTree(child);
	}
	std::cout << "}" << std::endl;
}

void deleteTree(Node *root) {
	for(Node * child : root->children) {
		deleteTree(child);
	}
	delete root;
}

int shortest(Node *root) {
	if(root->isLeaf()) return 0;
	int32_t minResult = (1 << 31) - 1; // std numeric limits
	for(Node * child : root->children) {
		minResult = std::min<int>(minResult, shortest(child) + 1);
	}
	return minResult;
}

int straightPath(Node *root) { // essentially cout leaves
	if(root->isLeaf()) return 1;
	int res = 0;
	for(Node * child : root->children) {
		res += straightPath(child);
	}
	return res;
}

template<size_t size>
void hideouts_helper(Node *root, Lab<size> &lab) {
	lab[root->position] = 1;
	for(Node * child : root->children) {
		hideouts_helper(child, lab);
	}
}

template<size_t size>
int hideouts(Node *root) {
	Lab<size> lab;
	hideouts_helper(root, lab);

	std::size_t count = 0;
	for(std::size_t i = 0; i < size; ++ i) {
		for(std::size_t j = 0; j < size; ++ j) {
			if(!lab[Point(i,j)]) ++count;
		}
	}
	return count;
}

int main() { 
	auto *t = tree(Lab<3>{{{0, 0, 0}, {1, 0, 0}, {0, 0, 0}}}, {0, 0}, {2, 2});

	printTree(t);

	std::cout << "shortest: " << shortest(t) << std::endl;
	std::cout << "straightPath: " << straightPath(t) << std::endl;
	std::cout << "hideouts: " << hideouts<3>(t) << std::endl;

	deleteTree(t);
}
