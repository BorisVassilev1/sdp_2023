#pragma once

#include <queue>
#include "TotalSSFT.hpp"
#include "concepts.hpp"

namespace fl {

template <fl::isLetter Letter>
class ComposeTotalSSFT : public TotalSSFT<Letter> {
   public:
	ComposeTotalSSFT(const TotalSSFT<Letter> &first, const TotalSSFT<Letter> &second) {
		using State = typename TotalSSFT<Letter>::State;

		struct BigState {
			State firstState;
			State secondState;
		};

		std::unordered_map<BigState, State> stateRemap;
		std::vector<bool>	 visited;
		auto								stateID = [&](const BigState &state) -> State {
			auto it = stateRemap.find(state);
			if (it != stateRemap.end()) return it->second;
			State newID		  = this->N++;
			stateRemap[state] = newID;
			this->transitions.emplace_back();
			visited.push_back(false);
			assert(this->transitions.size() == this->N);
			assert(visited.size() == this->N);
			return newID;
		};

		auto createTransition = [&](const BigState &state, Letter letter) -> BigState {
			const auto &[s1, s2] = state;
			State next_s1 = s1, next_s2 = s2;
			auto [o1, succ1] = first.step(next_s1, letter);
			if (!succ1) return {-1u, -1u};	   // maybe wrong
			static std::vector<Letter> output;
			bool					   succ2 = second.step(next_s2, o1, output);
			if (!succ2) return {-1u, -1u};	   // maybe wrong

			BigState next_state{next_s1, next_s2};
			State	 currentID							 = stateID(state);
			State	 nextID								 = stateID(next_state);
			this->transitions[currentID][(size_t)letter] = {this->words.add(output), nextID};
			output.clear();
			return next_state;
		};

		this->initialState = BigState{first.initialState, second.initialState};
		// bfs generation
		std::queue<BigState> q;
		q.push(this->initialState);
		while (!q.empty()) {
			BigState current = q.front();
			q.pop();
			if(visited[stateID(current)]) continue;
			visited[stateID(current)] = true;
			State currentID = stateID(current);
			for (Letter letter : Letter::all()) {
				BigState next = createTransition(current, letter);
				if (next.first != -1u && next.second != -1u) { q.push(next); }
			}
		}
	}
};

};	   // namespace fl
