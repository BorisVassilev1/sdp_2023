#include "bst.h"

int main() {
	BST<int> bst;
	bst.insert(1);
	bst.insert(2);
	bst.insert(3);
	bst.insert(4);
	bst.insert(0);
	bst.balance();

	bst.print();
	std::cout << std::endl;

	std::cout << bst.height() << std::endl; 

}
