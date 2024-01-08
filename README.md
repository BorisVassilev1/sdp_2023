# LL(1) Grammar Parser
## Project for "Data Structures Programming" course at FMI

This project is a library that defines Context-Free Grammar, Deterministic PushDown Automaton, and Parser (Parse-Tree builder) as C++ classes.

The main functionality can be described by this example:
```cpp
	CFG<Letter> g;
	g.terminals	   = {'a', 'b', '#'};
	g.nonTerminals = {'S'};
	g.addRule('S', "aSb");
	g.addRule('S', {});
	g.start = 'S';

	Parser<State, Letter> a(g);
	std::cout << a.parse("aabb") << std::endl;
```
This produces the following parse-tree:
```
-S
 ├-a
 ├-S
 │ ├-a
 │ ├-S
 │ │ └-ε
 │ └-b
 └-b
```
A more complex example can be seen in [language.cpp](https://github.com/BorisVassilev1/sdp_2023/blob/master/DPDA/language.cpp).

