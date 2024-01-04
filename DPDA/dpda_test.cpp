#include <initializer_list>
#include <iostream>
#include <unordered_set>
#include <assert.h>
#include <unordered_set>
#include "dpda.h"

#define s	 0
#define _f	 1
#define f(x) x + Letter::size
#define eps	 Letter::eps

void test_anbn_1() {
	/*
	 * S -> aSb | eps
	 */
	DPDA<State, Letter> a;
	a.addTransition(0, 'a', eps, 0, {'a'});
	a.addTransition(0, 'a', 'a', 0, {'a', 'a'});
	a.addTransition(0, 'b', 'a', 1, {});
	a.addTransition(1, 'b', 'a', 1, {});
	a.addTransition(1, '\0', eps, 2, {});
	a.qFinal = 2;

	std::string str = "aaaaaabbbbbb";
	assert(a.recognize(str) == true);
}

void test_arith() {
	/* SIGMA = {i, +, (, ), ., #}
	 *
	 * e -> e+t | t
	 * t -> t.f | f
	 * f -> i | (e)
	 *
	 *
	 * modified:
	 *  e -> tE
	 *  E -> eps | +tE
	 *  t -> FT
	 *  T -> eps | .fT
	 *  f -> i | (e)
	 */

	std::vector<char> Sigma = {'i', '+', '(', ')', '.', '#'};

	DPDA<State, Letter> a;
	a.addTransition(s, eps, eps, _f, "e");

	for (char x : Sigma)
		a.addTransition(_f, x, eps, f(x), {});

	for (char x : {'i', '+', '(', ')', '.'})
		a.addTransition(f(x), eps, x, _f, {});

	for (char x : Sigma) {
		a.addTransition(f(x), eps, 'e', f(x), "tE");
	}
	a.addTransition(f('+'), eps, 'E', f('+'), "+tE");

	for (char x : {')', '#'}) {
		a.addTransition(f(x), eps, 'E', f(x), {});
	}

	for (char x : Sigma) {
		a.addTransition(f(x), eps, 't', f(x), "fT");
	}
	a.addTransition(f('.'), eps, 'T', f('.'), ".fT");

	for (char x : {'+', ')', '#'}) {
		a.addTransition(f(x), eps, 'T', f(x), {});
	}
	a.addTransition(f('('), eps, 'f', f('('), "(e)");
	a.addTransition(f('i'), eps, 'f', f('i'), "i");
	a.qFinal = f('#');
    // a.enable_print = true;

	std::string str1 = "(i+i).i#";
	auto pt1 = a.parse(str1);
	assert(pt1 != nullptr);
	std::cout << pt1 << std::endl;
	deleteParseTree(pt1);

	std::string str2 = "(i+i).i.(i.(i+.)).(i+i+i)#";
	assert(a.recognize(str2) == false);
	std::string str3 = "(i+i).i.(i.(i+i+i+i)).(i+i+i)#";
	assert(a.recognize(str3) == true);
}

void test_anbn_2() {
	/*
	 * S -> aSb | eps
	 */
	DPDA<State, Letter> a;
	a.addTransition(s, eps, eps, _f, "S");
	a.addTransition(_f, '0', eps, f('0'), {});
	a.addTransition(f('0'), eps, '0', _f, {});
	a.addTransition(_f, '1', eps, f('1'), {});
	a.addTransition(f('1'), eps, '1', _f, {});
	a.addTransition(f('0'), eps, 'S', f('0'), "0S1");
	a.addTransition(f('1'), eps, 'S', f('1'), {});
	a.addTransition(_f, '#', eps, f('#'), {});
	a.qFinal = f('#');

	std::string str1 = "000111#";
	assert(a.recognize(str1) == true);
	std::string str2 = "001111#";
	assert(a.recognize(str2) == false);
}
#undef eps
#undef f
#undef s
#undef _f

void test_arith_gen() {
	CFG<Letter> g;
	g.terminals	   = {'i', '(', ')', '.', '+', '#'};
	g.nonTerminals = {'e', 'E', 't', 'T', 'f'};
	g.addRule('e', "tE");
	g.addRule('E', "");
	g.addRule('E', "+tE");
	g.addRule('t', "fT");
	g.addRule('T', "");
	g.addRule('T', ".fT");
	g.addRule('f', "(e)");
	g.addRule('f', "i");
	g.start = 'e';
	g.eof	= '#';

	DPDA<State, Letter> a(g);
	// a.printTransitions();
	// a.enable_print = true;
	std::string str1 = "(i+i).i#";
	ParseNode<Letter> *pt1 = a.parse(str1);
	assert(pt1 != nullptr);
	std::cout << pt1 << std::endl;
	deleteParseTree(pt1);
	
	std::string str2 = "(i+i).i.(i.(i+.)).(i+i+i)#";
	assert(a.recognize(str2) == false);
	std::string str3 = "(i+i).i.(i.(i+i+i+i)).(i+i+i)#";
	assert(a.recognize(str3) == true);
}

void test_ll1_1() {
	CFG<Letter> g;
	g.terminals	   = {'a', 'b', 'c', 'd', '#'};
	g.nonTerminals = {'S', 'A', 'B'};
	g.addRule('S', "aABb");
	g.addRule('A', "c");
	g.addRule('A', "");
	g.addRule('B', "d");
	g.addRule('B', "");
	g.start = 'S';
	g.eof	= '#';

	DPDA<State, Letter> a(g);
	// a.printTransitions();
	// a.enable_print = true;
	std::string str1 = "acdb#";
	auto pt1 = a.parse(str1);
	assert(pt1 != nullptr);
	std::cout << pt1 << std::endl;
	deleteParseTree(pt1);
}

void test_ll1_2() {
	CFG<Letter> g;
	g.terminals	   = {'a', 'b', 'c', 'd', 'e', 'f', '#'};
	g.nonTerminals = {'S', 'A', 'B', 'C', 'D'};
	g.addRule('S', "AB");
	g.addRule('S', "eDa");
	g.addRule('A', "ab");
	g.addRule('A', "c");
	g.addRule('B', "dC");
	g.addRule('C', "eC");
	g.addRule('C', "");
	g.addRule('D', "fD");
	g.addRule('D', "");
	g.start = 'S';
	g.eof	= '#';

	DPDA<State, Letter> a(g);
	//a.printTransitions();
	//a.enable_print = true;
	std::string str1 = "effffb#";
	std::string str2 = "abdfeeee#";
	std::string str3 = "effffa#";
	std::string str4 = "abdeeee#";
	assert(a.recognize(str1) == false);
	assert(a.recognize(str2) == false);

	auto pt3 = a.parse(str3);
	assert(pt3 != nullptr);
	std::cout << pt3 << std::endl;
	deleteParseTree(pt3);

	auto pt4 = a.parse(str4);
	assert(pt4 != nullptr);
	std::cout << pt4 << std::endl;
	deleteParseTree(pt4);
}

int main() {
	test_anbn_1();
	test_anbn_2();
	test_arith();
	test_arith_gen();
	test_ll1_1();
	test_ll1_2();
}
