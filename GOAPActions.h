#pragma once
#include <unordered_map>
#include "SteeringHelpers.h"
#include "IExamInterface.h"
#include "structs.h"

class Agent;
class Blackboard;
class GOAPPlanner;
class IExamInterface;

class GOAPAction
{
public:
	GOAPAction(GOAPPlanner* pPlanner);
	virtual ~GOAPAction();

	// Plan the action
	virtual bool Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) = 0;
	// Prepare the action
	virtual void Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) {};
	// Perform the action
	virtual bool Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt) { return true; };

	std::vector<GOAPProperty*> GetPreconditions() { return m_Preconditions; };
	std::vector<GOAPProperty*> GetEffects() { return m_Effects; };
	bool HasEffect(GOAPProperty* pPrecondition);

	float GetCost() { return m_Cost; };
	virtual Elite::Vector2 GetMoveLocation() { return moveTarget.Position; };

	virtual bool RequiresMovement(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const { return false; }; // If yes, the GoTo state will be ran before going into Perform
	virtual bool IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const { return true; };
protected:
	// Conditions that have to be met in order to start this action
	std::vector<GOAPProperty*> m_Preconditions;
	// Effects that will be applied to the world state after completing this action
	std::vector<GOAPProperty*> m_Effects;

	// Cost of the action
	float m_Cost = 1.f;

	// Requires the FSM to move to a location before performing
	bool m_RequiresMovement = false;
	TargetData moveTarget{};

	virtual void InitPreConditions(GOAPPlanner* pPlanner) = 0;
	virtual void InitEffects(GOAPPlanner* pPlanner) = 0;

	// TODO: Apply effects?
	virtual bool CheckEffects(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) { return true; };

	virtual bool CheckPreConditions(GOAPPlanner* pPlanner) const { return true; }; // List of pre defined pre conditions that have to be met
	virtual bool CheckProceduralPreconditions(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) { return true; }; // Procedually check the world for conditions

	// Cleanup
	virtual void Cleanup();
};

class GOAPSurvive : public GOAPAction
{
public:
	GOAPSurvive(GOAPPlanner* pPlanner);
	virtual bool Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard);
	virtual void Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard);
private:
	virtual void InitPreConditions(GOAPPlanner* pPlanner);
	virtual void InitEffects(GOAPPlanner* pPlanner);
};

class GOAPExploreWorldAction final : public GOAPAction
{
public:
	GOAPExploreWorldAction(GOAPPlanner* pPlanner);
	virtual bool Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard);
	virtual void Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) override;
	virtual bool RequiresMovement(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const override;
private:
	float m_ExploreActionRange{ 5.f };
	float m_MovementFulfilledRange{ 3.f };
	std::vector<Elite::Vector2> m_LocationsOfInterest{};

	virtual void InitPreConditions(GOAPPlanner* pPlanner) override;
	virtual void InitEffects(GOAPPlanner* pPlanner) override;
};

class GOAPFindGeneralHouseLocationsAction final : public GOAPAction
{
public:
	GOAPFindGeneralHouseLocationsAction(GOAPPlanner* pPlanner);
	virtual bool Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard);
	virtual void Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) override;
	virtual bool Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt) override;

	virtual bool RequiresMovement(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const override;
	virtual bool IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const override;
private:
	float m_ExploreVicinityRadius{ 200.f };
	float m_ExploreActionRange{ 5.f };
	float m_MovementFulfilledRange{ 3.f };

	float m_Angle{ 0.f }, m_AngleIncrement{ 5.f };
	float m_IgnoreLocationDistance{ 5.f };
	std::vector<Elite::Vector2> m_LocationsOfInterest{};

	virtual void InitPreConditions(GOAPPlanner* pPlanner) override;
	virtual void InitEffects(GOAPPlanner* pPlanner) override;

	virtual bool CheckEffects(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) override;

	virtual bool CheckPreConditions(GOAPPlanner* pPlanner) const override;
	virtual bool CheckProceduralPreconditions(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) override;
};

class GOAPEvadeEnemy final : public GOAPAction
{
public:
	GOAPEvadeEnemy(GOAPPlanner* pPlanner);
	virtual bool Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard);
	virtual void Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) override;
	virtual bool Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt) override;

	virtual bool RequiresMovement(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const override { return false; };
	virtual bool IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const override;
private:
	float m_EvadeTime = 2.f, m_EvadeTimer = 0.f;

	virtual void InitPreConditions(GOAPPlanner* pPlanner);
	virtual void InitEffects(GOAPPlanner* pPlanner);

	virtual bool CheckEffects(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) { return true; };
	virtual bool CheckPreConditions(GOAPPlanner* pPlanner) const override { return true; };
	virtual bool CheckProceduralPreconditions(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) override { return true; };
};