#include "stdafx.h"
#include "GOAPActions.h"
#include "GOAPPlanner.h"
#include "IExamInterface.h"
#include "Blackboard.h"
#include "WorldState.h"
#include "Agent.h"
#include "utils.h"

// ---------------------------
// Base class GOAPAction
GOAPAction::GOAPAction(GOAPPlanner* pPlanner)
{
	m_pWorldState = pPlanner->GetWorldState();
}
GOAPAction::~GOAPAction()
{
	Cleanup();
}
bool GOAPAction::HasEffect(GOAPProperty* pPrecondition)
{
	bool hasEffect{ false };
	for (GOAPProperty* pEffect : m_Effects)
	{
		if (pEffect->propertyKey == pPrecondition->propertyKey)
		{
			hasEffect = true;
		}
	}
	return hasEffect;
}
void GOAPAction::Cleanup()
{
	for (GOAPProperty* pProperty : m_Preconditions)
	{
		delete pProperty;
		pProperty = nullptr;
	}
	for (GOAPProperty* pProperty : m_Effects)
	{
		delete pProperty;
		pProperty = nullptr;
	}
}
// ---------------------------

// GOAPSurvive, GOAL NODE for planner
// Preconditions: HasMoreThan5Energy(true), HasMoreThan5Health(true)
	// NoEnemiesInSpotted (true), have distant goal of finding weapon and stocking up on items
// Effects: None
GOAPSurvive::GOAPSurvive(GOAPPlanner* pPlanner) :
	GOAPAction(pPlanner)
{
	m_RequiresMovement = true;
	InitPreConditions(pPlanner);
	InitEffects(pPlanner);
}
bool GOAPSurvive::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPSurvive::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	std::cout << "Setting up GOAPSurvive\n";
}
void GOAPSurvive::InitPreConditions(GOAPPlanner* pPlanner)
{
	GOAPProperty* pEnergyCondition = new GOAPProperty{ "HasMoreThan5Energy", true };
	utils::AddActionProperty(pEnergyCondition, m_Preconditions, m_pWorldState, false);

	GOAPProperty* pHealthCondition = new GOAPProperty{ "HasMoreThan5Health", true };
	utils::AddActionProperty(pHealthCondition, m_Preconditions, m_pWorldState, true);

	//GOAPProperty* pTestCondition = new GOAPProperty{ "SurviveTest", true };
	//utils::AddActionProperty(pTestCondition, m_Preconditions, m_pWorldState, false);
}
void GOAPSurvive::InitEffects(GOAPPlanner* pPlanner) {}

// DrinkEnergy
// Preconditions: HasEnergyItem(true)
// Effects: HasMoreThan5Energy(true), HasEnergyItem(false)
GOAPDrinkEnergy::GOAPDrinkEnergy(GOAPPlanner* pPlanner) :
	GOAPAction(pPlanner)
{
	InitPreConditions(pPlanner);
	InitEffects(pPlanner);
}
bool GOAPDrinkEnergy::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPDrinkEnergy::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	std::cout << "Setting up GOAPDrinkEnergy\n";
}
bool GOAPDrinkEnergy::Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt)
{
	return true;
}
void GOAPDrinkEnergy::InitPreConditions(GOAPPlanner* pPlanner)
{
	GOAPProperty* pCondition = new GOAPProperty{ "HasEnergyItem", true };
	utils::AddActionProperty(pCondition, m_Preconditions, m_pWorldState, false);
}
void GOAPDrinkEnergy::InitEffects(GOAPPlanner* pPlanner)
{
	GOAPProperty* pEnergyCondition = new GOAPProperty{ "HasMoreThan5Energy", true };
	utils::AddActionProperty(pEnergyCondition, m_Effects, m_pWorldState, true);
}

// GOAPSearchItem: public GOAPAction
GOAPSearchItem::GOAPSearchItem(GOAPPlanner* pPlanner) :
	GOAPAction(pPlanner)
{}
bool GOAPSearchItem::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPSearchItem::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	std::cout << "Setting up GOAPSearchItem\n";
	// Setup behavior to an item search behavior with priority for energy
	bool dataValid = pBlackboard->GetData("HouseCornerLocations", m_pHouseCornerLocations)
		&& pBlackboard->GetData("HouseLocations", m_pHouseLocations)
		&& pBlackboard->GetData("ItemLocations", m_pItemsOnGround)
		&& pBlackboard->GetData("Agent", m_pAgent);

	if (!dataValid)
	{
		std::cout << "Error obtaining blackboard data in GOAPSearchItem::Setup\n";
		return;
	}

	ChooseSeekLocation(pInterface, pPlanner, pBlackboard);
	m_pAgent->SetBehavior(BehaviorType::SEEKDODGE);
}
bool GOAPSearchItem::Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt)
{
	auto vEntitiesInFov = utils::GetEntitiesInFOV(pInterface);
	auto vHousesInFOV = utils::GetHousesInFOV(pInterface);

	for (EntityInfo& entity : vEntitiesInFov)
	{
		if (entity.Type == eEntityType::ITEM)
		{
			ItemInfo itemInfo;
			pInterface->Item_GetInfo(entity, itemInfo);
			auto foundIt = std::find_if(m_pItemsOnGround->begin(), m_pItemsOnGround->end(), [&itemInfo](ItemInfo& item)
				{
					return item.Location == itemInfo.Location;
				}
			);

			// New item found!
			if (foundIt == m_pItemsOnGround->end())
			{
				std::cout << "Item found!\n";
				m_pItemsOnGround->push_back(itemInfo);
			}
		}
	}

	// Go over all houses in vision
	for (HouseInfo& house : vHousesInFOV)
	{
		// See if we have already memorized the house location
		auto foundIterator = std::find_if(m_pHouseLocations->begin(), m_pHouseLocations->end(), [&house](ExploredHouse& exploredHouse)
			{
				return house.Center == exploredHouse.houseInfo.Center;
			}
		);

		// House does not exist yet
		if (foundIterator == m_pHouseLocations->end())
		{
			// Add the house to the known locations
			m_pHouseLocations->push_back(ExploredHouse{ house, FLT_MAX });
			std::cout << "Houses explored: " << m_pHouseLocations->size() << "\n";
			// Remove all corner locations of this house
			RemoveExploredCornerLocations(house);
			// Choose a new seek location
			ChooseSeekLocation(pInterface, pPlanner, pBlackboard);
		}
	}

	if (CheckArrival(pInterface, pPlanner, pBlackboard))
	{
		ChooseSeekLocation(pInterface, pPlanner, pBlackboard);
	}

	// Debug seek location
	pInterface->Draw_SolidCircle(m_pAgent->GetGoalPosition(), 3.f, {}, { 0.f,1.f,1.f });

	// Debug corner locations
	for (const Elite::Vector2& c : *m_pHouseCornerLocations)
	{
		pInterface->Draw_SolidCircle(c, 2.f, {}, { 0.f,0.f,1.f });
	}
	pInterface->Draw_SolidCircle(distantGoalPos, 2.f, {}, { 1.f, 0.f, 0.f });

	return true;
}
bool GOAPSearchItem::IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const
{
	return false;
}
void GOAPSearchItem::ChooseSeekLocation(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	Elite::Vector2 destination{};
	const Elite::Vector2& agentPos = pInterface->Agent_GetInfo().Position;

	std::vector<ExploredHouse*> possibleHouses{};
	for (ExploredHouse& h : *m_pHouseLocations)
	{
		if (h.timeSinceExplored > m_HouseExploreCooldown)
			possibleHouses.push_back(&h);
	}

	if (possibleHouses.size() > 0)
	{
		float closestHouseDistanceFromAgentSquared{ FLT_MAX };
		ExploredHouse* pClosestHouse = nullptr;
		for (ExploredHouse* h : possibleHouses)
		{
			float distanceToAgentSquared = agentPos.DistanceSquared(h->houseInfo.Center);
			if (distanceToAgentSquared < closestHouseDistanceFromAgentSquared)
			{
				closestHouseDistanceFromAgentSquared = distanceToAgentSquared;
				pClosestHouse = h;
			}
		}

		if (pClosestHouse)
		{
			distantGoalPos = pClosestHouse->houseInfo.Center;
			destination = pInterface->NavMesh_GetClosestPathPoint(pClosestHouse->houseInfo.Center);
		}
		else
			std::cout << "Error finding path to house\n";
	}
	else
	{
		float closestCornerDistanceFromAgentSquared{ FLT_MAX };
		Elite::Vector2 closestCorner{};
		if (m_pHouseCornerLocations->size() > 0)
		{
			for (const Elite::Vector2& cornerLoc : (*m_pHouseCornerLocations))
			{
				float distanceToAgentSquared = agentPos.DistanceSquared(cornerLoc);

				if (distanceToAgentSquared < closestCornerDistanceFromAgentSquared)
				{
					closestCornerDistanceFromAgentSquared = distanceToAgentSquared;
					closestCorner = cornerLoc;
				}
			}

			distantGoalPos = closestCorner;
			destination = pInterface->NavMesh_GetClosestPathPoint(closestCorner);
		}
		else
		{
			std::cout << "no more corners\n";
		}
	}

	m_pAgent->SetGoalPosition(destination);
}
bool GOAPSearchItem::CheckArrival(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	const Elite::Vector2& agentPos = pInterface->Agent_GetInfo().Position;

	//Check if agent is in house
	for (ExploredHouse& h : *m_pHouseLocations)
	{
		float housePadding{ 4.f };
		float marginX{ h.houseInfo.Size.x / housePadding };
		float marginY{ h.houseInfo.Size.y / housePadding };
		float halfWidth = h.houseInfo.Size.x / 2.f;
		float halfHeight = h.houseInfo.Size.y / 2.f;
		// Check if agent location is in the house
		if ((agentPos.x + housePadding < h.houseInfo.Center.x + halfWidth) && (agentPos.x - housePadding > h.houseInfo.Center.x - halfWidth) &&
			(agentPos.y + housePadding < h.houseInfo.Center.y + halfHeight) && (agentPos.y - housePadding > h.houseInfo.Center.y - halfHeight))
		{
			std::cout << "Agent is in house\n";
			h.timeSinceExplored = 0.f;
			return true;
		}
	}

	if (agentPos.DistanceSquared(m_pAgent->GetGoalPosition()) < m_ArrivalRange * m_ArrivalRange)
	{
		std::cout << "Arrived\n";
		return true;
	}

	return false;
}
void GOAPSearchItem::RemoveExploredCornerLocations(HouseInfo& houseInfo)
{
	// Remove the house corner location if it's in the explored house vicinity		
	auto findIt = std::find_if(m_pHouseCornerLocations->begin(), m_pHouseCornerLocations->end(), [houseInfo](Elite::Vector2& pos)
		{
			float margin{ 3.f };
			float halfWidth = houseInfo.Size.x / 2.f;
			float halfHeight = houseInfo.Size.y / 2.f;

			// Check if corner location is in the vicinity of a house
			if ((pos.x - margin < houseInfo.Center.x + halfWidth) && (pos.x + margin > houseInfo.Center.x - halfWidth) &&
				(pos.y - margin < houseInfo.Center.y + halfHeight) && (pos.y + margin > houseInfo.Center.y - halfHeight))
			{
				return true;
			}
			return false;
		}
	);

	if (findIt != m_pHouseCornerLocations->end())
	{
		m_pHouseCornerLocations->erase(findIt);
	}
}

// SearchForEnergy
// Preconditions: InitialHouseScoutDone(true) 
// Effects: HasEnergyItem(true)
GOAPSearchForEnergy::GOAPSearchForEnergy(GOAPPlanner* pPlanner) :
	GOAPSearchItem(pPlanner)
{
	InitPreConditions(pPlanner);
	InitEffects(pPlanner);
}
bool GOAPSearchForEnergy::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPSearchForEnergy::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	GOAPSearchItem::Setup(pInterface, pPlanner, pBlackboard);
}
bool GOAPSearchForEnergy::Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt)
{
	bool foundEnergy{ false };
	for (ItemInfo& i : *m_pItemsOnGround)
	{
		if (i.Type == eItemType::FOOD)
		{
			//foundEnergy = true;
		}
	}

	if (!foundEnergy)
	{
		bool searchItemSucceeded = GOAPSearchItem::Perform(pInterface, pPlanner, pBlackboard, dt);
		if (!searchItemSucceeded)
			return false;
	}

	// No problems were encountered
	return true;
}
bool GOAPSearchForEnergy::IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const
{
	// Check if we found energy item
	return GOAPSearchItem::IsDone(pInterface, pPlanner, pBlackboard);
}
void GOAPSearchForEnergy::InitPreConditions(GOAPPlanner* pPlanner)
{
	GOAPProperty* pCondition = new GOAPProperty{ "InitialHouseScoutDone", true };
	utils::AddActionProperty(pCondition, m_Preconditions, m_pWorldState, false);
}
void GOAPSearchForEnergy::InitEffects(GOAPPlanner* pPlanner)
{
	GOAPProperty* pCondition = new GOAPProperty{ "HasEnergyItem", true };
	utils::AddActionProperty(pCondition, m_Effects, m_pWorldState, false);
}

// Explore world action
// Preconditions: InitialHouseScoutDone(true) 
	//TODO: IsInHouse(false) 
// Effects: SurviveTest(true)
GOAPExploreWorldAction::GOAPExploreWorldAction(GOAPPlanner* pPlanner) :
	GOAPAction(pPlanner)
{
	std::cout << "GOAPExploreWorldAction constructed\n";
	m_RequiresMovement = true;

	InitPreConditions(pPlanner);
	InitEffects(pPlanner);
}
bool GOAPExploreWorldAction::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPExploreWorldAction::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	std::cout << "Setting up GOAPExploreWorldAction\n";
	// Plan where to move to considering the tile locations that have already been explored
	// Consider explored houses and a range around the houses for a more optimal search routine

	AgentInfo agentInfo = pInterface->Agent_GetInfo();
	moveTarget.Position = agentInfo.Position + Elite::Vector2(m_ExploreActionRange * 7.f, m_ExploreActionRange * 14.f);
}
bool GOAPExploreWorldAction::RequiresMovement(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const
{
	AgentInfo agentInfo = pInterface->Agent_GetInfo();
	Elite::Vector2 agentPosition = agentInfo.Position;
	float distanceToPosition = agentPosition.Distance(moveTarget.Position);

	// Check if the movement is fullfilled
	if (distanceToPosition < m_MovementFulfilledRange)
		return false;

	// Movement required
	return true;
}
void GOAPExploreWorldAction::InitPreConditions(GOAPPlanner* pPlanner)
{
	// Get a reference to the world states
	WorldState* pWorldState = pPlanner->GetWorldState();

	// Create states
	// House in sight state
	GOAPProperty* pHouseInSight = new GOAPProperty{ "InitialHouseScoutDone", true };
	m_Preconditions.push_back(pHouseInSight);

	// Make sure the states exist in the world
	if (!pWorldState->DoesStateExist(pHouseInSight->propertyKey))
	{
		// State doesn't exist, add the state with some default starter value
		pWorldState->AddState(pHouseInSight->propertyKey, false);
	}
}
void GOAPExploreWorldAction::InitEffects(GOAPPlanner* pPlanner)
{
	// Get a reference to the world states
	WorldState* pWorldState = pPlanner->GetWorldState();

	// Create states
	// House in sight state
	GOAPProperty* pHouseInSight = new GOAPProperty{ "SurviveTest", true };
	m_Effects.push_back(pHouseInSight);

	// Make sure the states exist in the world
	if (!pWorldState->DoesStateExist(pHouseInSight->propertyKey))
	{
		// State doesn't exist, add the state with some default starter value
		pWorldState->AddState(pHouseInSight->propertyKey, pHouseInSight->value.bValue);
	}
}

// Find general house locations action
// Preconditions: InitialHouseScoutDone(false)
// Effects: InitialHouseScoutDone(true)
GOAPFindGeneralHouseLocationsAction::GOAPFindGeneralHouseLocationsAction(GOAPPlanner* pPlanner) :
	GOAPAction(pPlanner)
{
	std::cout << "GOAPFindGeneralHouseLocationsAction constructed\n";
	m_Cost = -100.f;

	InitPreConditions(pPlanner);
	InitEffects(pPlanner);
}
bool GOAPFindGeneralHouseLocationsAction::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPFindGeneralHouseLocationsAction::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	std::cout << "Setting up GOAPFindGeneralHouseLocationsAction\n";
	pBlackboard->AddData("HouseCornerLocations", &m_HouseCornerLocations);
}
bool GOAPFindGeneralHouseLocationsAction::Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt)
{
	while (m_Angle < 360.f)
	{
		Elite::Vector2 locationToExplore{ cos(m_Angle) * m_ExploreVicinityRadius, sin(m_Angle) * m_ExploreVicinityRadius };
		Elite::Vector2 cornerLocation = pInterface->NavMesh_GetClosestPathPoint(locationToExplore);

		if (locationToExplore.Distance(cornerLocation) >= m_IgnoreLocationDistance)
		{
			bool exists = false;
			auto foundIt = std::find(m_HouseCornerLocations.begin(), m_HouseCornerLocations.end(), cornerLocation);

			// Check if this location was a new location
			if (foundIt == m_HouseCornerLocations.end())
			{
				m_HouseCornerLocations.push_back(cornerLocation);
			}
		}

		m_Angle += m_AngleIncrement;
	}

	return true;
}
bool GOAPFindGeneralHouseLocationsAction::CheckPreConditions(GOAPPlanner* pPlanner) const
{
	return false;
}
bool GOAPFindGeneralHouseLocationsAction::CheckProceduralPreconditions(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return false;
}
bool GOAPFindGeneralHouseLocationsAction::RequiresMovement(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const
{
	return false;
}
bool GOAPFindGeneralHouseLocationsAction::IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const
{
	// This action will always be done after 1 frame.
	// Apply effects
	for (GOAPProperty* pEffect : m_Effects)
	{
		m_pWorldState->SetState(pEffect->propertyKey, pEffect->value.bValue);
	}
	return true;
}
void GOAPFindGeneralHouseLocationsAction::InitPreConditions(GOAPPlanner* pPlanner)
{
	GOAPProperty* pCondition = new GOAPProperty{ "InitialHouseScoutDone", false };
	utils::AddActionProperty(pCondition, m_Preconditions, m_pWorldState, false);
}
void GOAPFindGeneralHouseLocationsAction::InitEffects(GOAPPlanner* pPlanner)
{
	GOAPProperty* pCondition = new GOAPProperty{ "InitialHouseScoutDone", true };
	utils::AddActionProperty(pCondition, m_Effects, m_pWorldState, false);
}
bool GOAPFindGeneralHouseLocationsAction::CheckEffects(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return false;
}

// TODO: make this a behavior
// GOAPEvadeEnemy
// Preconditions: EnemyWasInSight(true)
// Effects: EnemyWasInSight(false)
GOAPEvadeEnemy::GOAPEvadeEnemy(GOAPPlanner* pPlanner) :
	GOAPAction(pPlanner)
{
	std::cout << "GOAPEvadeEnemy constructed\n";
	m_Cost = 2.f;

	InitPreConditions(pPlanner);
	InitEffects(pPlanner);
}
bool GOAPEvadeEnemy::Plan(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	return true;
}
void GOAPEvadeEnemy::Setup(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard)
{
	std::cout << "Setting up GOAPEvadeEnemy\n";
	m_EvadeTimer = 0.f;

	Agent* pAgent = nullptr;
	// Check if agent data is valid
	bool dataValid = pBlackboard->GetData("Agent", pAgent);
	if (!dataValid) return;

	// Set agent to dodge behavior
	pAgent->SetBehavior(BehaviorType::SEEKDODGE);
}
bool GOAPEvadeEnemy::Perform(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard, float dt)
{
	m_EvadeTimer += dt;
	return true;
}
bool GOAPEvadeEnemy::IsDone(IExamInterface* pInterface, GOAPPlanner* pPlanner, Blackboard* pBlackboard) const
{
	if (m_EvadeTimer >= m_EvadeTime)
	{
		Agent* pAgent = nullptr;
		// Check if agent data is valid
		bool dataValid = pBlackboard->GetData("Agent", pAgent);
		if (!dataValid) return false;

		// Clear the agents behavior
		pAgent->ClearBehavior();

		return true;
	}

	return false;
}
void GOAPEvadeEnemy::InitPreConditions(GOAPPlanner* pPlanner)
{
	// Get a reference to the world states
	WorldState* pWorldState = pPlanner->GetWorldState();

	// Create states
	// Requires an enemy in sight to run
	GOAPProperty* pEnemyWasInSight = new GOAPProperty{ "EnemyWasInSight", true };
	m_Preconditions.push_back(pEnemyWasInSight);

	// Make sure the states exist in the world
	if (!pWorldState->DoesStateExist(pEnemyWasInSight->propertyKey))
	{
		// State doesn't exist, add the state with some default starter value
		pWorldState->AddState(pEnemyWasInSight->propertyKey, pEnemyWasInSight->value.bValue);
	}
}
void GOAPEvadeEnemy::InitEffects(GOAPPlanner* pPlanner)
{
	// Get a reference to the world states
	WorldState* pWorldState = pPlanner->GetWorldState();

	// Create states
	// Requires an enemy in sight to run
	GOAPProperty* pEnemyWasInSight = new GOAPProperty{ "EnemyWasInSight", false };
	m_Preconditions.push_back(pEnemyWasInSight);

	// Make sure the states exist in the world
	if (!pWorldState->DoesStateExist(pEnemyWasInSight->propertyKey))
	{
		// State doesn't exist, add the state with some default starter value
		pWorldState->AddState(pEnemyWasInSight->propertyKey, pEnemyWasInSight->value.bValue);
	}
}

