#pragma once
#include <queue>

class GOAPAction;
class WorldState;
class Blackboard;
class ActionSearchAlgorithm
{
public:
	ActionSearchAlgorithm(WorldState* pWorldState);
	std::queue<GOAPAction*> Search(GOAPAction* pGoalAction, std::vector<GOAPAction*> possibleActions);
private:
	WorldState* m_pWorldState = nullptr;
};

