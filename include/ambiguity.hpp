#pragma once

#include <stack>
#include "FST.hpp"

namespace fl {

template <class Letter>
void tarjan(int u, const typename FST<Letter>::Map &transitions);

static int				 foundat = 1, sccIndex = 0;
static std::vector<int>	 scc;
static std::vector<int>	 disc, low;		// init disc to -1
static std::vector<bool> onstack;		// init to 0

template <class Letter>
void tarjan(int u, const typename FST<Letter>::Map &transitions) {
	static std::stack<int> st;

	disc[u] = low[u] = foundat++;
	st.push(u);
	onstack[u]	   = true;
	auto [it, end] = transitions.equal_range(u);
	for (auto iit = it; iit != end; ++iit) {
		auto &[id1, id2, i] = iit->second;
		if (id1 != 0) continue;
		if (disc[i] == -1) {
			tarjan<Letter>(i, transitions);
			low[u] = std::min(low[u], low[i]);
		} else if (onstack[i]) low[u] = std::min(low[u], disc[i]);
	}
	if (disc[u] == low[u]) {
		while (1) {
			int v = st.top();
			st.pop();
			onstack[v] = false;
			scc[v]	   = sccIndex;
			if (u == v) break;
		}
		++sccIndex;
	}
}

template <class Letter>
bool testInfiniteAmbiguity(const FST<Letter> &fst) {
	// tarjan algorithm to find strongly connected components
	// we search in the subgraph with transitions only <\varepsilon, w>

	if (fst.transitions.empty()) return false;

	disc.assign(fst.N, -1);
	low.assign(fst.N, -1);
	onstack.assign(fst.N, false);
	scc.assign(fst.N, -1);

	tarjan<Letter>(0, fst.transitions);

	for (const auto &[k, v] : fst.transitions) {
		auto &[id1, id2, i] = v;
		if (id1 != 0 || id2 == 0) continue;		// transition is (\varepsilon, w)
		if (i == k) return true;
		if (i != k && scc[i] == scc[k] && scc[i] != -1) {
			return true;	 // found a cycle in the epsilon transitions
		}
	}

	return false;
}
}	  // namespace fl
