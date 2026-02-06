// OdysseyAutomationNetworkSystem.cpp
// Implementation of mass production automation system

#include "OdysseyAutomationNetworkSystem.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyInventoryComponent.h"

UOdysseyAutomationNetworkSystem::UOdysseyAutomationNetworkSystem()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;

	NetworkUpdateFrequency = 0.1f;
	MaxNodesInNetwork = 100;
	bAutoDetectBottlenecks = true;
	BottleneckDetectionInterval = 5.0f;

	TimeSinceLastUpdate = 0.0f;
	TimeSinceLastBottleneckCheck = 0.0f;

	CraftingManager = nullptr;
	InventoryComponent = nullptr;
}

void UOdysseyAutomationNetworkSystem::BeginPlay()
{
	Super::BeginPlay();
}

void UOdysseyAutomationNetworkSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastUpdate += DeltaTime;
	TimeSinceLastBottleneckCheck += DeltaTime;

	// Process network at fixed intervals for mobile optimization
	if (TimeSinceLastUpdate >= NetworkUpdateFrequency)
	{
		ProcessNetwork(TimeSinceLastUpdate);
		TimeSinceLastUpdate = 0.0f;
	}

	// Periodic bottleneck detection
	if (bAutoDetectBottlenecks && TimeSinceLastBottleneckCheck >= BottleneckDetectionInterval)
	{
		CheckForBottlenecks();
		TimeSinceLastBottleneckCheck = 0.0f;
	}
}

// ============================================================================
// Node Management
// ============================================================================

FGuid UOdysseyAutomationNetworkSystem::CreateNode(EAutomationNodeType NodeType, FVector Position, FString Name)
{
	if (Nodes.Num() >= MaxNodesInNetwork)
	{
		return FGuid();
	}

	FAutomationNode NewNode = CreateDefaultNode(NodeType);
	NewNode.WorldPosition = Position;

	if (!Name.IsEmpty())
	{
		NewNode.NodeName = Name;
	}

	Nodes.Add(NewNode.NodeID, NewNode);
	UpdateStatistics();

	return NewNode.NodeID;
}

bool UOdysseyAutomationNetworkSystem::RemoveNode(FGuid NodeID)
{
	if (!Nodes.Contains(NodeID))
	{
		return false;
	}

	// Remove all connections to/from this node
	TArray<FGuid> ConnectionsToRemove;
	for (const auto& Pair : Connections)
	{
		if (Pair.Value.SourceNodeID == NodeID || Pair.Value.TargetNodeID == NodeID)
		{
			ConnectionsToRemove.Add(Pair.Key);
		}
	}

	for (const FGuid& ConnID : ConnectionsToRemove)
	{
		Connections.Remove(ConnID);
	}

	// Remove from production lines
	for (auto& LinePair : ProductionLines)
	{
		LinePair.Value.NodeIDs.Remove(NodeID);
	}

	Nodes.Remove(NodeID);
	UpdateStatistics();

	return true;
}

FAutomationNode UOdysseyAutomationNetworkSystem::GetNode(FGuid NodeID) const
{
	const FAutomationNode* Node = Nodes.Find(NodeID);
	return Node ? *Node : FAutomationNode();
}

TArray<FAutomationNode> UOdysseyAutomationNetworkSystem::GetAllNodes() const
{
	TArray<FAutomationNode> AllNodes;
	for (const auto& Pair : Nodes)
	{
		AllNodes.Add(Pair.Value);
	}
	return AllNodes;
}

bool UOdysseyAutomationNetworkSystem::UpdateNode(FGuid NodeID, const FAutomationNode& UpdatedNode)
{
	FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return false;
	}

	*Node = UpdatedNode;
	Node->NodeID = NodeID; // Preserve ID

	return true;
}

bool UOdysseyAutomationNetworkSystem::SetNodeEnabled(FGuid NodeID, bool bEnabled)
{
	FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return false;
	}

	EAutomationNodeState OldState = Node->CurrentState;
	Node->CurrentState = bEnabled ? EAutomationNodeState::Idle : EAutomationNodeState::Disabled;

	if (OldState != Node->CurrentState)
	{
		OnNodeStateChanged.Broadcast(NodeID, Node->CurrentState);
	}

	return true;
}

bool UOdysseyAutomationNetworkSystem::AssignRecipeToNode(FGuid NodeID, FName RecipeID)
{
	FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return false;
	}

	if (Node->NodeType != EAutomationNodeType::Processing)
	{
		return false;
	}

	Node->AssignedRecipe = RecipeID;

	return true;
}

// ============================================================================
// Connection Management
// ============================================================================

FGuid UOdysseyAutomationNetworkSystem::CreateConnection(FGuid SourceNodeID, int32 SourceSlot, FGuid TargetNodeID, int32 TargetSlot)
{
	// Validate nodes exist
	FAutomationNode* SourceNode = Nodes.Find(SourceNodeID);
	FAutomationNode* TargetNode = Nodes.Find(TargetNodeID);

	if (!SourceNode || !TargetNode)
	{
		return FGuid();
	}

	// Validate slots
	if (SourceSlot < 0 || SourceSlot >= SourceNode->OutputSlots ||
		TargetSlot < 0 || TargetSlot >= TargetNode->InputSlots)
	{
		return FGuid();
	}

	// Check for cycles
	if (HasCycle(SourceNodeID, TargetNodeID))
	{
		return FGuid();
	}

	// Create connection
	FAutomationConnection NewConnection;
	NewConnection.ConnectionID = FGuid::NewGuid();
	NewConnection.SourceNodeID = SourceNodeID;
	NewConnection.TargetNodeID = TargetNodeID;
	NewConnection.SourceSlot = SourceSlot;
	NewConnection.TargetSlot = TargetSlot;
	NewConnection.bIsActive = true;

	Connections.Add(NewConnection.ConnectionID, NewConnection);

	// Update node connection lists
	SourceNode->OutputConnections.Add(NewConnection.ConnectionID);
	TargetNode->InputConnections.Add(NewConnection.ConnectionID);

	UpdateStatistics();

	return NewConnection.ConnectionID;
}

bool UOdysseyAutomationNetworkSystem::RemoveConnection(FGuid ConnectionID)
{
	FAutomationConnection* Connection = Connections.Find(ConnectionID);
	if (!Connection)
	{
		return false;
	}

	// Remove from node connection lists
	FAutomationNode* SourceNode = Nodes.Find(Connection->SourceNodeID);
	FAutomationNode* TargetNode = Nodes.Find(Connection->TargetNodeID);

	if (SourceNode)
	{
		SourceNode->OutputConnections.Remove(ConnectionID);
	}
	if (TargetNode)
	{
		TargetNode->InputConnections.Remove(ConnectionID);
	}

	Connections.Remove(ConnectionID);
	UpdateStatistics();

	return true;
}

FAutomationConnection UOdysseyAutomationNetworkSystem::GetConnection(FGuid ConnectionID) const
{
	const FAutomationConnection* Connection = Connections.Find(ConnectionID);
	return Connection ? *Connection : FAutomationConnection();
}

TArray<FAutomationConnection> UOdysseyAutomationNetworkSystem::GetAllConnections() const
{
	TArray<FAutomationConnection> AllConnections;
	for (const auto& Pair : Connections)
	{
		AllConnections.Add(Pair.Value);
	}
	return AllConnections;
}

bool UOdysseyAutomationNetworkSystem::SetConnectionFilter(FGuid ConnectionID, const TArray<EResourceType>& AllowedResources)
{
	FAutomationConnection* Connection = Connections.Find(ConnectionID);
	if (!Connection)
	{
		return false;
	}

	Connection->FilteredResources = AllowedResources;
	return true;
}

bool UOdysseyAutomationNetworkSystem::SetConnectionTransferRate(FGuid ConnectionID, float NewRate)
{
	FAutomationConnection* Connection = Connections.Find(ConnectionID);
	if (!Connection)
	{
		return false;
	}

	Connection->TransferRate = FMath::Max(NewRate, 0.1f);
	return true;
}

bool UOdysseyAutomationNetworkSystem::ValidateConnection(FGuid SourceNodeID, FGuid TargetNodeID) const
{
	if (!Nodes.Contains(SourceNodeID) || !Nodes.Contains(TargetNodeID))
	{
		return false;
	}

	if (SourceNodeID == TargetNodeID)
	{
		return false;
	}

	return !HasCycle(SourceNodeID, TargetNodeID);
}

// ============================================================================
// Production Line Management
// ============================================================================

FGuid UOdysseyAutomationNetworkSystem::CreateProductionLine(const TArray<FGuid>& NodeIDs, FString LineName)
{
	if (NodeIDs.Num() == 0)
	{
		return FGuid();
	}

	// Validate all nodes exist
	for (const FGuid& NodeID : NodeIDs)
	{
		if (!Nodes.Contains(NodeID))
		{
			return FGuid();
		}
	}

	FProductionLine NewLine;
	NewLine.LineID = FGuid::NewGuid();
	NewLine.LineName = LineName.IsEmpty() ? TEXT("Production Line") : LineName;
	NewLine.NodeIDs = NodeIDs;
	NewLine.bIsActive = true;

	// Determine final product from output nodes
	for (const FGuid& NodeID : NodeIDs)
	{
		const FAutomationNode* Node = Nodes.Find(NodeID);
		if (Node && Node->NodeType == EAutomationNodeType::Output && !Node->AssignedRecipe.IsNone())
		{
			NewLine.FinalProduct = Node->AssignedRecipe;
			break;
		}
	}

	ProductionLines.Add(NewLine.LineID, NewLine);
	UpdateProductionLineMetrics(ProductionLines[NewLine.LineID]);

	return NewLine.LineID;
}

FGuid UOdysseyAutomationNetworkSystem::AutoCreateProductionLine(FGuid OutputNodeID)
{
	const FAutomationNode* OutputNode = Nodes.Find(OutputNodeID);
	if (!OutputNode || OutputNode->NodeType != EAutomationNodeType::Output)
	{
		return FGuid();
	}

	// Trace upstream to find all connected nodes
	TArray<FGuid> LineNodes;
	TSet<FGuid> Visited;
	TArray<FGuid> ToVisit;
	ToVisit.Add(OutputNodeID);

	while (ToVisit.Num() > 0)
	{
		FGuid Current = ToVisit.Pop();
		if (Visited.Contains(Current))
		{
			continue;
		}

		Visited.Add(Current);
		LineNodes.Add(Current);

		TArray<FGuid> Upstream = GetUpstreamNodes(Current);
		for (const FGuid& UpstreamID : Upstream)
		{
			if (!Visited.Contains(UpstreamID))
			{
				ToVisit.Add(UpstreamID);
			}
		}
	}

	return CreateProductionLine(LineNodes, TEXT("Auto-Generated Line"));
}

bool UOdysseyAutomationNetworkSystem::RemoveProductionLine(FGuid LineID)
{
	return ProductionLines.Remove(LineID) > 0;
}

FProductionLine UOdysseyAutomationNetworkSystem::GetProductionLine(FGuid LineID) const
{
	const FProductionLine* Line = ProductionLines.Find(LineID);
	return Line ? *Line : FProductionLine();
}

TArray<FProductionLine> UOdysseyAutomationNetworkSystem::GetAllProductionLines() const
{
	TArray<FProductionLine> AllLines;
	for (const auto& Pair : ProductionLines)
	{
		AllLines.Add(Pair.Value);
	}
	return AllLines;
}

bool UOdysseyAutomationNetworkSystem::SetProductionLineActive(FGuid LineID, bool bActive)
{
	FProductionLine* Line = ProductionLines.Find(LineID);
	if (!Line)
	{
		return false;
	}

	bool bOldActive = Line->bIsActive;
	Line->bIsActive = bActive;

	if (bOldActive != bActive)
	{
		// Enable/disable all nodes in the line
		for (const FGuid& NodeID : Line->NodeIDs)
		{
			SetNodeEnabled(NodeID, bActive);
		}

		OnProductionLineStatusChanged.Broadcast(LineID, bActive);
	}

	return true;
}

// ============================================================================
// Resource Flow
// ============================================================================

bool UOdysseyAutomationNetworkSystem::InjectResources(FGuid NodeID, EResourceType ResourceType, int32 Amount)
{
	FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return false;
	}

	if (Node->NodeType != EAutomationNodeType::Input && Node->NodeType != EAutomationNodeType::Storage)
	{
		return false;
	}

	return Node->InputBuffer.Add(ResourceType, Amount);
}

int32 UOdysseyAutomationNetworkSystem::ExtractResources(FGuid NodeID, EResourceType ResourceType, int32 MaxAmount)
{
	FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return 0;
	}

	if (Node->NodeType != EAutomationNodeType::Output && Node->NodeType != EAutomationNodeType::Storage)
	{
		return 0;
	}

	return Node->OutputBuffer.Remove(ResourceType, MaxAmount);
}

float UOdysseyAutomationNetworkSystem::GetConnectionFlowRate(FGuid ConnectionID) const
{
	const FAutomationConnection* Connection = Connections.Find(ConnectionID);
	return Connection ? Connection->CurrentFlow : 0.0f;
}

FResourceBuffer UOdysseyAutomationNetworkSystem::GetNodeInputBuffer(FGuid NodeID) const
{
	const FAutomationNode* Node = Nodes.Find(NodeID);
	return Node ? Node->InputBuffer : FResourceBuffer();
}

FResourceBuffer UOdysseyAutomationNetworkSystem::GetNodeOutputBuffer(FGuid NodeID) const
{
	const FAutomationNode* Node = Nodes.Find(NodeID);
	return Node ? Node->OutputBuffer : FResourceBuffer();
}

// ============================================================================
// Analysis and Optimization
// ============================================================================

FBottleneckAnalysis UOdysseyAutomationNetworkSystem::AnalyzeBottlenecks(FGuid LineID) const
{
	FBottleneckAnalysis Analysis;

	const FProductionLine* Line = ProductionLines.Find(LineID);
	if (!Line)
	{
		return Analysis;
	}

	FGuid BottleneckNode;
	float LowestEfficiency = 1.0f;

	for (const FGuid& NodeID : Line->NodeIDs)
	{
		const FAutomationNode* Node = Nodes.Find(NodeID);
		if (!Node)
		{
			continue;
		}

		// Check for starved nodes (not getting enough input)
		if (Node->CurrentState == EAutomationNodeState::Starved)
		{
			if (Node->Efficiency < LowestEfficiency)
			{
				LowestEfficiency = Node->Efficiency;
				BottleneckNode = NodeID;
				Analysis.BottleneckReason = TEXT("Node is starved - not receiving enough input resources");
			}
		}

		// Check for blocked nodes (output buffer full)
		if (Node->CurrentState == EAutomationNodeState::Blocked)
		{
			if (Node->Efficiency < LowestEfficiency)
			{
				LowestEfficiency = Node->Efficiency;
				BottleneckNode = NodeID;
				Analysis.BottleneckReason = TEXT("Node is blocked - output buffer is full");
			}
		}

		// Check for low efficiency processing nodes
		if (Node->NodeType == EAutomationNodeType::Processing && Node->Efficiency < LowestEfficiency)
		{
			LowestEfficiency = Node->Efficiency;
			BottleneckNode = NodeID;
			Analysis.BottleneckReason = TEXT("Processing node has low efficiency");
		}
	}

	Analysis.BottleneckNodeID = BottleneckNode;
	Analysis.SeverityScore = 1.0f - LowestEfficiency;
	Analysis.PotentialEfficiencyGain = (1.0f / FMath::Max(LowestEfficiency, 0.1f)) - 1.0f;

	// Generate recommendations
	if (Analysis.SeverityScore > 0.5f)
	{
		Analysis.Recommendations.Add(TEXT("Consider adding parallel processing nodes to reduce bottleneck"));
		Analysis.Recommendations.Add(TEXT("Increase buffer sizes on affected nodes"));
	}
	if (Analysis.SeverityScore > 0.3f)
	{
		Analysis.Recommendations.Add(TEXT("Optimize upstream production to match processing rate"));
		Analysis.Recommendations.Add(TEXT("Check connection transfer rates"));
	}

	return Analysis;
}

TArray<FString> UOdysseyAutomationNetworkSystem::GetOptimizationRecommendations(FGuid LineID) const
{
	TArray<FString> Recommendations;

	FBottleneckAnalysis Analysis = AnalyzeBottlenecks(LineID);
	Recommendations.Append(Analysis.Recommendations);

	const FProductionLine* Line = ProductionLines.Find(LineID);
	if (!Line)
	{
		return Recommendations;
	}

	// Additional recommendations based on line structure
	int32 ProcessingNodes = 0;
	int32 StorageNodes = 0;

	for (const FGuid& NodeID : Line->NodeIDs)
	{
		const FAutomationNode* Node = Nodes.Find(NodeID);
		if (!Node) continue;

		if (Node->NodeType == EAutomationNodeType::Processing) ProcessingNodes++;
		if (Node->NodeType == EAutomationNodeType::Storage) StorageNodes++;
	}

	if (StorageNodes == 0 && ProcessingNodes > 2)
	{
		Recommendations.Add(TEXT("Add storage nodes between processing stages to buffer resources"));
	}

	if (Line->OverallEfficiency < 0.8f)
	{
		Recommendations.Add(TEXT("Overall efficiency is below 80% - consider balancing production rates"));
	}

	return Recommendations;
}

float UOdysseyAutomationNetworkSystem::CalculateMaxThroughput(FGuid LineID) const
{
	const FProductionLine* Line = ProductionLines.Find(LineID);
	if (!Line)
	{
		return 0.0f;
	}

	float MinThroughput = FLT_MAX;

	for (const FGuid& NodeID : Line->NodeIDs)
	{
		const FAutomationNode* Node = Nodes.Find(NodeID);
		if (!Node || Node->NodeType != EAutomationNodeType::Processing)
		{
			continue;
		}

		// Calculate theoretical throughput for this node
		float NodeThroughput = Node->ProcessingSpeed * Node->BatchSize;
		MinThroughput = FMath::Min(MinThroughput, NodeThroughput);
	}

	return MinThroughput == FLT_MAX ? 0.0f : MinThroughput;
}

float UOdysseyAutomationNetworkSystem::GetProductionLineEfficiency(FGuid LineID) const
{
	const FProductionLine* Line = ProductionLines.Find(LineID);
	return Line ? Line->OverallEfficiency : 0.0f;
}

// ============================================================================
// Statistics
// ============================================================================

FAutomationNetworkStats UOdysseyAutomationNetworkSystem::GetNetworkStatistics() const
{
	return Statistics;
}

FAutomationNode UOdysseyAutomationNetworkSystem::GetNodeMetrics(FGuid NodeID) const
{
	return GetNode(NodeID);
}

void UOdysseyAutomationNetworkSystem::ResetStatistics()
{
	Statistics = FAutomationNetworkStats();

	for (auto& Pair : Nodes)
	{
		Pair.Value.TotalItemsProcessed = 0;
		Pair.Value.ThroughputRate = 0.0f;
	}
}

// ============================================================================
// Integration
// ============================================================================

void UOdysseyAutomationNetworkSystem::SetCraftingManager(UOdysseyCraftingManager* NewManager)
{
	CraftingManager = NewManager;
}

void UOdysseyAutomationNetworkSystem::SetInventoryComponent(UOdysseyInventoryComponent* NewInventory)
{
	InventoryComponent = NewInventory;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UOdysseyAutomationNetworkSystem::ProcessNetwork(float DeltaTime)
{
	// First, process resource flow through connections
	ProcessResourceFlow(DeltaTime);

	// Then process each node
	for (auto& Pair : Nodes)
	{
		ProcessNode(Pair.Value, DeltaTime);
	}

	// Update production line metrics
	for (auto& LinePair : ProductionLines)
	{
		if (LinePair.Value.bIsActive)
		{
			UpdateProductionLineMetrics(LinePair.Value);
		}
	}

	// Update overall statistics
	UpdateStatistics();
}

void UOdysseyAutomationNetworkSystem::ProcessNode(FAutomationNode& Node, float DeltaTime)
{
	if (Node.CurrentState == EAutomationNodeState::Disabled || !Node.bHasPower)
	{
		return;
	}

	UpdateNodeState(Node);

	switch (Node.NodeType)
	{
	case EAutomationNodeType::Processing:
		{
			// Check if we have ingredients and recipe
			if (Node.AssignedRecipe.IsNone() || Node.CurrentState == EAutomationNodeState::Starved)
			{
				return;
			}

			// Process crafting
			if (CraftingManager)
			{
				FAdvancedCraftingRecipe Recipe = CraftingManager->GetRecipe(Node.AssignedRecipe);
				
				// Check if we have required ingredients in input buffer
				bool bHasIngredients = true;
				for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
				{
					if (Node.InputBuffer.GetAmount(Ingredient.ResourceType) < Ingredient.Amount * Node.BatchSize)
					{
						bHasIngredients = false;
						break;
					}
				}

				if (bHasIngredients && Node.CurrentState != EAutomationNodeState::Blocked)
				{
					// Progress crafting
					float CraftingRate = Node.ProcessingSpeed * DeltaTime / Recipe.BaseCraftingTime;
					Node.CurrentProgress += CraftingRate;

					if (Node.CurrentProgress >= 1.0f)
					{
						// Consume ingredients
						for (const FCraftingIngredient& Ingredient : Recipe.PrimaryIngredients)
						{
							Node.InputBuffer.Remove(Ingredient.ResourceType, Ingredient.Amount * Node.BatchSize);
						}

						// Produce outputs
						for (const FCraftingOutput& Output : Recipe.PrimaryOutputs)
						{
							Node.OutputBuffer.Add(Output.ResourceType, Output.Amount * Node.BatchSize);
						}

						Node.TotalItemsProcessed += Node.BatchSize;
						Node.CurrentProgress = 0.0f;

						OnProductionCompleted.Broadcast(Node.NodeID, Node.AssignedRecipe);
					}
				}
			}
		}
		break;

	case EAutomationNodeType::Splitter:
		// Distribute input to multiple outputs evenly
		// Resources flow is handled in ProcessResourceFlow
		break;

	case EAutomationNodeType::Merger:
		// Combine multiple inputs to single output
		// Resources flow is handled in ProcessResourceFlow
		break;

	case EAutomationNodeType::Filter:
		// Only allow specific resource types through
		// Handled by connection filters
		break;

	default:
		break;
	}

	// Update efficiency metric
	if (Node.NodeType == EAutomationNodeType::Processing)
	{
		float InputFill = Node.InputBuffer.GetFillRatio();
		float OutputFill = 1.0f - Node.OutputBuffer.GetFillRatio();
		Node.Efficiency = FMath::Min(InputFill, OutputFill);
	}
}

void UOdysseyAutomationNetworkSystem::ProcessResourceFlow(float DeltaTime)
{
	for (auto& Pair : Connections)
	{
		if (Pair.Value.bIsActive)
		{
			TransferResources(Pair.Value, DeltaTime);
		}
	}
}

void UOdysseyAutomationNetworkSystem::TransferResources(FAutomationConnection& Connection, float DeltaTime)
{
	FAutomationNode* SourceNode = Nodes.Find(Connection.SourceNodeID);
	FAutomationNode* TargetNode = Nodes.Find(Connection.TargetNodeID);

	if (!SourceNode || !TargetNode)
	{
		return;
	}

	// Calculate transfer amount based on rate and delta time
	int32 MaxTransfer = FMath::CeilToInt(Connection.TransferRate * DeltaTime);
	Connection.CurrentFlow = 0.0f;

	// Transfer each resource type in source output buffer
	for (auto& ResourcePair : SourceNode->OutputBuffer.Resources)
	{
		EResourceType ResourceType = ResourcePair.Key;
		int32 Available = ResourcePair.Value;

		if (Available <= 0)
		{
			continue;
		}

		// Check filter
		if (Connection.FilteredResources.Num() > 0 && !Connection.FilteredResources.Contains(ResourceType))
		{
			continue;
		}

		// Calculate actual transfer
		int32 ToTransfer = FMath::Min(Available, MaxTransfer);
		if (TargetNode->InputBuffer.CanAdd(ResourceType, ToTransfer))
		{
			int32 Transferred = SourceNode->OutputBuffer.Remove(ResourceType, ToTransfer);
			TargetNode->InputBuffer.Add(ResourceType, Transferred);
			Connection.CurrentFlow += Transferred;

			if (Transferred > 0)
			{
				OnResourceTransferred.Broadcast(Connection.ConnectionID, ResourceType, Transferred);
			}
		}
	}
}

void UOdysseyAutomationNetworkSystem::UpdateNodeState(FAutomationNode& Node)
{
	if (Node.CurrentState == EAutomationNodeState::Disabled)
	{
		return;
	}

	EAutomationNodeState OldState = Node.CurrentState;
	EAutomationNodeState NewState = EAutomationNodeState::Active;

	// Check for starved (input buffer too low)
	if (Node.NodeType == EAutomationNodeType::Processing)
	{
		if (Node.InputBuffer.IsEmpty() || Node.InputBuffer.GetFillRatio() < 0.1f)
		{
			NewState = EAutomationNodeState::Starved;
		}
		// Check for blocked (output buffer full)
		else if (Node.OutputBuffer.IsFull() || Node.OutputBuffer.GetFillRatio() > 0.9f)
		{
			NewState = EAutomationNodeState::Blocked;
		}
	}

	// Check power
	if (!Node.bHasPower)
	{
		NewState = EAutomationNodeState::Error;
	}

	if (NewState != OldState)
	{
		Node.CurrentState = NewState;
		OnNodeStateChanged.Broadcast(Node.NodeID, NewState);
	}
}

void UOdysseyAutomationNetworkSystem::UpdateProductionLineMetrics(FProductionLine& Line)
{
	if (Line.NodeIDs.Num() == 0)
	{
		return;
	}

	float TotalEfficiency = 0.0f;
	float MinEfficiency = 1.0f;
	int32 TotalEnergy = 0;
	FGuid WorstNode;

	for (const FGuid& NodeID : Line.NodeIDs)
	{
		const FAutomationNode* Node = Nodes.Find(NodeID);
		if (!Node)
		{
			continue;
		}

		TotalEfficiency += Node->Efficiency;
		TotalEnergy += Node->EnergyConsumption;

		if (Node->Efficiency < MinEfficiency)
		{
			MinEfficiency = Node->Efficiency;
			WorstNode = NodeID;
		}
	}

	Line.OverallEfficiency = TotalEfficiency / Line.NodeIDs.Num();
	Line.TotalEnergyConsumption = TotalEnergy;
	Line.BottleneckNodeID = WorstNode;
	Line.ProductionRate = CalculateMaxThroughput(Line.LineID) * Line.OverallEfficiency;
}

void UOdysseyAutomationNetworkSystem::CheckForBottlenecks()
{
	for (const auto& LinePair : ProductionLines)
	{
		if (!LinePair.Value.bIsActive)
		{
			continue;
		}

		FBottleneckAnalysis Analysis = AnalyzeBottlenecks(LinePair.Key);
		if (Analysis.SeverityScore > 0.3f)
		{
			OnBottleneckDetected.Broadcast(LinePair.Key, Analysis);
		}
	}
}

void UOdysseyAutomationNetworkSystem::UpdateStatistics()
{
	Statistics.TotalNodes = Nodes.Num();
	Statistics.TotalConnections = Connections.Num();
	Statistics.ActiveNodes = 0;
	Statistics.TotalEnergyConsumption = 0;
	Statistics.TotalThroughput = 0.0f;

	float TotalEfficiency = 0.0f;
	int32 ActiveCount = 0;

	for (const auto& Pair : Nodes)
	{
		const FAutomationNode& Node = Pair.Value;

		if (Node.CurrentState == EAutomationNodeState::Active ||
			Node.CurrentState == EAutomationNodeState::Starved ||
			Node.CurrentState == EAutomationNodeState::Blocked)
		{
			Statistics.ActiveNodes++;
			TotalEfficiency += Node.Efficiency;
			ActiveCount++;
		}

		Statistics.TotalEnergyConsumption += Node.EnergyConsumption;
		Statistics.TotalThroughput += Node.ThroughputRate;
		Statistics.TotalItemsProduced += Node.TotalItemsProcessed;
	}

	Statistics.AverageEfficiency = ActiveCount > 0 ? TotalEfficiency / ActiveCount : 0.0f;
}

TArray<FGuid> UOdysseyAutomationNetworkSystem::GetUpstreamNodes(FGuid NodeID) const
{
	TArray<FGuid> Upstream;

	const FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return Upstream;
	}

	for (const FGuid& ConnID : Node->InputConnections)
	{
		const FAutomationConnection* Connection = Connections.Find(ConnID);
		if (Connection)
		{
			Upstream.Add(Connection->SourceNodeID);
		}
	}

	return Upstream;
}

TArray<FGuid> UOdysseyAutomationNetworkSystem::GetDownstreamNodes(FGuid NodeID) const
{
	TArray<FGuid> Downstream;

	const FAutomationNode* Node = Nodes.Find(NodeID);
	if (!Node)
	{
		return Downstream;
	}

	for (const FGuid& ConnID : Node->OutputConnections)
	{
		const FAutomationConnection* Connection = Connections.Find(ConnID);
		if (Connection)
		{
			Downstream.Add(Connection->TargetNodeID);
		}
	}

	return Downstream;
}

bool UOdysseyAutomationNetworkSystem::HasCycle(FGuid StartNodeID, FGuid EndNodeID) const
{
	// DFS to check if there's a path from EndNodeID back to StartNodeID
	TSet<FGuid> Visited;
	TArray<FGuid> Stack;
	Stack.Add(EndNodeID);

	while (Stack.Num() > 0)
	{
		FGuid Current = Stack.Pop();

		if (Current == StartNodeID)
		{
			return true;
		}

		if (Visited.Contains(Current))
		{
			continue;
		}

		Visited.Add(Current);

		TArray<FGuid> Downstream = GetDownstreamNodes(Current);
		for (const FGuid& NextID : Downstream)
		{
			if (!Visited.Contains(NextID))
			{
				Stack.Add(NextID);
			}
		}
	}

	return false;
}

FAutomationNode UOdysseyAutomationNetworkSystem::CreateDefaultNode(EAutomationNodeType NodeType) const
{
	FAutomationNode Node;
	Node.NodeType = NodeType;

	switch (NodeType)
	{
	case EAutomationNodeType::Input:
		Node.NodeName = TEXT("Input Node");
		Node.InputSlots = 0;
		Node.OutputSlots = 1;
		Node.InputBuffer.MaxCapacity = 500;
		Node.OutputBuffer.MaxCapacity = 100;
		Node.EnergyConsumption = 5;
		break;

	case EAutomationNodeType::Output:
		Node.NodeName = TEXT("Output Node");
		Node.InputSlots = 1;
		Node.OutputSlots = 0;
		Node.InputBuffer.MaxCapacity = 100;
		Node.OutputBuffer.MaxCapacity = 500;
		Node.EnergyConsumption = 5;
		break;

	case EAutomationNodeType::Processing:
		Node.NodeName = TEXT("Processing Node");
		Node.InputSlots = 2;
		Node.OutputSlots = 2;
		Node.InputBuffer.MaxCapacity = 100;
		Node.OutputBuffer.MaxCapacity = 100;
		Node.ProcessingSpeed = 1.0f;
		Node.BatchSize = 1;
		Node.EnergyConsumption = 20;
		break;

	case EAutomationNodeType::Storage:
		Node.NodeName = TEXT("Storage Node");
		Node.InputSlots = 2;
		Node.OutputSlots = 2;
		Node.InputBuffer.MaxCapacity = 1000;
		Node.OutputBuffer.MaxCapacity = 1000;
		Node.EnergyConsumption = 2;
		break;

	case EAutomationNodeType::Splitter:
		Node.NodeName = TEXT("Splitter Node");
		Node.InputSlots = 1;
		Node.OutputSlots = 3;
		Node.InputBuffer.MaxCapacity = 50;
		Node.OutputBuffer.MaxCapacity = 50;
		Node.EnergyConsumption = 3;
		break;

	case EAutomationNodeType::Merger:
		Node.NodeName = TEXT("Merger Node");
		Node.InputSlots = 3;
		Node.OutputSlots = 1;
		Node.InputBuffer.MaxCapacity = 50;
		Node.OutputBuffer.MaxCapacity = 50;
		Node.EnergyConsumption = 3;
		break;

	case EAutomationNodeType::Filter:
		Node.NodeName = TEXT("Filter Node");
		Node.InputSlots = 1;
		Node.OutputSlots = 2;
		Node.InputBuffer.MaxCapacity = 50;
		Node.OutputBuffer.MaxCapacity = 50;
		Node.EnergyConsumption = 3;
		break;
	}

	return Node;
}
