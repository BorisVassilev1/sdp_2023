#include <initializer_list>
#include <iostream>
#include <unordered_set>
#include <assert.h>
#include <unordered_set>
#include "dpda.h"

#define s	 0
#define _f	 1
#define f(x) x + 128
#define eps	 Letter::eps

void test_anbn_1() {
	/*
	 * S -> aSb | eps
	 */
	DPDA<std::size_t, Letter> a;
	a.addTransition(0, 'a', eps, 0, {'a'});
	a.addTransition(0, 'a', 'a', 0, {'a', 'a'});
	a.addTransition(0, 'b', 'a', 1, {});
	a.addTransition(1, 'b', 'a', 1, {});
	a.addTransition(1, '\0', eps, 2, {});
	a.qFinal = 2;

	std::string str = "aaaaaabbbbbb";
	assert(a.parse(str) == true);
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

	DPDA<std::size_t, Letter> a;
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

	std::string str1 = "(i+i).i#";
	assert(a.parse(str1) == true);
	std::string str2 = "(i+i).i.(i.(i+.)).(i+i+i)#";
	assert(a.parse(str2) == false);
	std::string str3 = "(i+i).i.(i.(i+i+i+i)).(i+i+i)#";
	assert(a.parse(str3) == true);
}

void test_anbn_2() {
	/*
	 * S -> aSb | eps
	 */
	DPDA<std::size_t, Letter> a;
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
	assert(a.parse(str1) == true);
	std::string str2 = "001111#";
	assert(a.parse(str2) == false);
}
#undef eps
#undef f
#undef s
#undef _f

void test_regular_grammar() {
	CFG<Letter> cfg;
	cfg.terminals	 = {'a', 'b', 'c', 'd', 'e', '#'};
	cfg.nonTerminals = {'A', 'B', 'C', 'D', 'E'};
	cfg.start		 = 'A';
	cfg.rules.insert({'A', {'a', 'b', 'A'}});
	cfg.rules.insert({'A', {'#'}});

	DPDA<std::size_t, Letter> a(cfg);
	a.printTransitions();
	a.enable_print = true;
	std::string				  str1 = "#";
	assert(a.parse(str1) == true);
}

int main() {
	test_anbn_1();
	test_anbn_2();
	test_arith();
	test_regular_grammar();
}
