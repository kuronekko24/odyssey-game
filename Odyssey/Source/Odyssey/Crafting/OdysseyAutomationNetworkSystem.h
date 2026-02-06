// OdysseyAutomationNetworkSystem.h
// Mass production automation system with resource flow management

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdysseyCraftingManager.h"
#include "OdysseyAutomationNetworkSystem.generated.h"

// Forward declarations
class UOdysseyCraftingManager;
class UOdysseyInventoryComponent;

/**
 * Automation node types
 */
UENUM(BlueprintType)
enum class EAutomationNodeType : uint8
{
	Input = 0       UMETA(DisplayName = "Input Node"),
	Output = 1      UMETA(DisplayName = "Output Node"),
	Processing = 2  UMETA(DisplayName = "Processing Node"),
	Storage = 3     UMETA(DisplayName = "Storage Node"),
	Splitter = 4    UMETA(DisplayName = "Splitter Node"),
	Merger = 5      UMETA(DisplayName = "Merger Node"),
	Filter = 6      UMETA(DisplayName = "Filter Node")
};

/**
 * Automation node state
 */
UENUM(BlueprintType)
enum class EAutomationNodeState : uint8
{
	Idle = 0        UMETA(DisplayName = "Idle"),
	Active = 1      UMETA(DisplayName = "Active"),
	Blocked = 2     UMETA(DisplayName = "Blocked"),
	Starved = 3     UMETA(DisplayName = "Starved"),
	Error = 4       UMETA(DisplayName = "Error"),
	Disabled = 5    UMETA(DisplayName = "Disabled")
};

/**
 * Resource flow connection between nodes
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FAutomationConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	FGuid ConnectionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	FGuid SourceNodeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	FGuid TargetNodeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	int32 SourceSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	int32 TargetSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	float TransferRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	float CurrentFlow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	TArray<EResourceType> FilteredResources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	bool bIsActive;

	FAutomationConnection()
	{
		ConnectionID = FGuid::NewGuid();
		SourceNodeID = FGuid();
		TargetNodeID = FGuid();
		SourceSlot = 0;
		TargetSlot = 0;
		TransferRate = 10.0f; // Units per second
		CurrentFlow = 0.0f;
		bIsActive = true;
	}
};

/**
 * Buffer for storing resources in transit
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FResourceBuffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffer")
	TMap<EResourceType, int32> Resources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffer")
	int32 MaxCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffer")
	int32 CurrentTotal;

	FResourceBuffer()
	{
		MaxCapacity = 100;
		CurrentTotal = 0;
	}

	bool CanAdd(EResourceType Type, int32 Amount) const
	{
		return (CurrentTotal + Amount) <= MaxCapacity;
	}

	bool Add(EResourceType Type, int32 Amount)
	{
		if (!CanAdd(Type, Amount))
		{
			return false;
		}
		int32& Count = Resources.FindOrAdd(Type);
		Count += Amount;
		CurrentTotal += Amount;
		return true;
	}

	int32 Remove(EResourceType Type, int32 Amount)
	{
		int32* Count = Resources.Find(Type);
		if (!Count || *Count <= 0)
		{
			return 0;
		}
		int32 Removed = FMath::Min(Amount, *Count);
		*Count -= Removed;
		CurrentTotal -= Removed;
		return Removed;
	}

	int32 GetAmount(EResourceType Type) const
	{
		const int32* Count = Resources.Find(Type);
		return Count ? *Count : 0;
	}

	bool IsEmpty() const { return CurrentTotal <= 0; }
	bool IsFull() const { return CurrentTotal >= MaxCapacity; }
	float GetFillRatio() const { return MaxCapacity > 0 ? static_cast<float>(CurrentTotal) / MaxCapacity : 0.0f; }
};

/**
 * Automation node for production networks
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FAutomationNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
	FGuid NodeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
	FString NodeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
	EAutomationNodeType NodeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
	EAutomationNodeState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Node")
	FVector WorldPosition;

	// Processing configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	FName AssignedRecipe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	float ProcessingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	float CurrentProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
	int32 BatchSize;

	// Connection slots
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connections")
	int32 InputSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connections")
	int32 OutputSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connections")
	TArray<FGuid> InputConnections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connections")
	TArray<FGuid> OutputConnections;

	// Buffers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffers")
	FResourceBuffer InputBuffer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buffers")
	FResourceBuffer OutputBuffer;

	// Performance metrics
	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float Efficiency;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float ThroughputRate;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float UptimeRatio;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 TotalItemsProcessed;

	// Energy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
	int32 EnergyConsumption;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
	bool bHasPower;

	FAutomationNode()
	{
		NodeID = FGuid::NewGuid();
		NodeName = TEXT("Automation Node");
		NodeType = EAutomationNodeType::Processing;
		CurrentState = EAutomationNodeState::Idle;
		WorldPosition = FVector::ZeroVector;
		AssignedRecipe = NAME_None;
		ProcessingSpeed = 1.0f;
		CurrentProgress = 0.0f;
		BatchSize = 1;
		InputSlots = 1;
		OutputSlots = 1;
		Efficiency = 1.0f;
		ThroughputRate = 0.0f;
		UptimeRatio = 1.0f;
		TotalItemsProcessed = 0;
		EnergyConsumption = 10;
		bHasPower = true;
	}
};

/**
 * Production line grouping multiple nodes
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FProductionLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production Line")
	FGuid LineID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production Line")
	FString LineName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production Line")
	TArray<FGuid> NodeIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production Line")
	FName FinalProduct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production Line")
	bool bIsActive;

	// Performance metrics
	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float OverallEfficiency;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	float ProductionRate;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	int32 TotalEnergyConsumption;

	UPROPERTY(BlueprintReadOnly, Category = "Metrics")
	FGuid BottleneckNodeID;

	FProductionLine()
	{
		LineID = FGuid::NewGuid();
		LineName = TEXT("Production Line");
		FinalProduct = NAME_None;
		bIsActive = true;
		OverallEfficiency = 1.0f;
		ProductionRate = 0.0f;
		TotalEnergyConsumption = 0;
	}
};

/**
 * Bottleneck analysis result
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FBottleneckAnalysis
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FGuid BottleneckNodeID;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString BottleneckReason;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float SeverityScore;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> Recommendations;

	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float PotentialEfficiencyGain;

	FBottleneckAnalysis()
	{
		SeverityScore = 0.0f;
		PotentialEfficiencyGain = 0.0f;
	}
};

/**
 * Network statistics
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FAutomationNetworkStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ActiveNodes;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalConnections;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float AverageEfficiency;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float TotalThroughput;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalEnergyConsumption;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalItemsProduced;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TMap<EResourceType, int32> ResourcesProducedByType;

	FAutomationNetworkStats()
	{
		TotalNodes = 0;
		ActiveNodes = 0;
		TotalConnections = 0;
		AverageEfficiency = 0.0f;
		TotalThroughput = 0.0f;
		TotalEnergyConsumption = 0;
		TotalItemsProduced = 0;
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeStateChanged, FGuid, NodeID, EAutomationNodeState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResourceTransferred, FGuid, ConnectionID, EResourceType, ResourceType, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProductionCompleted, FGuid, NodeID, FName, RecipeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBottleneckDetected, FGuid, LineID, const FBottleneckAnalysis&, Analysis);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProductionLineStatusChanged, FGuid, LineID, bool, bIsActive);

/**
 * Automation Network System
 * 
 * Manages automated production networks:
 * - Node-based production graph
 * - Resource flow between nodes
 * - Bottleneck detection and optimization suggestions
 * - Production line management
 * - Energy consumption tracking
 */
UCLASS(ClassGroup=(Odyssey), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ODYSSEY_API UOdysseyAutomationNetworkSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdysseyAutomationNetworkSystem();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================================
	// Node Management
	// ============================================================================

	/**
	 * Create a new automation node
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	FGuid CreateNode(EAutomationNodeType NodeType, FVector Position, FString Name = TEXT(""));

	/**
	 * Remove a node and all its connections
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	bool RemoveNode(FGuid NodeID);

	/**
	 * Get node by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	FAutomationNode GetNode(FGuid NodeID) const;

	/**
	 * Get all nodes
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	TArray<FAutomationNode> GetAllNodes() const;

	/**
	 * Update node configuration
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	bool UpdateNode(FGuid NodeID, const FAutomationNode& UpdatedNode);

	/**
	 * Set node enabled state
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	bool SetNodeEnabled(FGuid NodeID, bool bEnabled);

	/**
	 * Assign recipe to processing node
	 */
	UFUNCTION(BlueprintCallable, Category = "Nodes")
	bool AssignRecipeToNode(FGuid NodeID, FName RecipeID);

	// ============================================================================
	// Connection Management
	// ============================================================================

	/**
	 * Create connection between nodes
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	FGuid CreateConnection(FGuid SourceNodeID, int32 SourceSlot, FGuid TargetNodeID, int32 TargetSlot);

	/**
	 * Remove connection
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	bool RemoveConnection(FGuid ConnectionID);

	/**
	 * Get connection by ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	FAutomationConnection GetConnection(FGuid ConnectionID) const;

	/**
	 * Get all connections
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	TArray<FAutomationConnection> GetAllConnections() const;

	/**
	 * Set connection filter (which resources can flow)
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	bool SetConnectionFilter(FGuid ConnectionID, const TArray<EResourceType>& AllowedResources);

	/**
	 * Set connection transfer rate
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	bool SetConnectionTransferRate(FGuid ConnectionID, float NewRate);

	/**
	 * Validate connection (check for cycles, type compatibility)
	 */
	UFUNCTION(BlueprintCallable, Category = "Connections")
	bool ValidateConnection(FGuid SourceNodeID, FGuid TargetNodeID) const;

	// ============================================================================
	// Production Line Management
	// ============================================================================

	/**
	 * Create a production line from connected nodes
	 */
	UFUNCTION(BlueprintCallable, Category = "Production Lines")
	FGuid CreateProductionLine(const TArray<FGuid>& NodeIDs, FString LineName);

	/**
	 * Auto-detect production line from a final output node
	 */
	UFUNCTION(BlueprintCallable, Category = "Production Lines")
	FGuid AutoCreateProductionLine(FGuid OutputNodeID);

	/**
	 * Remove production line
	 */
	UFUNCTION(BlueprintCallable, Category = "Production Lines")
	bool RemoveProductionLine(FGuid LineID);

	/**
	 * Get production line
	 */
	UFUNCTION(BlueprintCallable, Category = "Production Lines")
	FProductionLine GetProductionLine(FGuid LineID) const;

	/**
	 * Get all production lines
	 */
	UFUNCTION(BlueprintCallable, Category = "Production Lines")
	TArray<FProductionLine> GetAllProductionLines() const;

	/**
	 * Set production line active state
	 */
	UFUNCTION(BlueprintCallable, Category = "Production Lines")
	bool SetProductionLineActive(FGuid LineID, bool bActive);

	// ============================================================================
	// Resource Flow
	// ============================================================================

	/**
	 * Manually inject resources into an input node
	 */
	UFUNCTION(BlueprintCallable, Category = "Resource Flow")
	bool InjectResources(FGuid NodeID, EResourceType ResourceType, int32 Amount);

	/**
	 * Extract resources from an output node
	 */
	UFUNCTION(BlueprintCallable, Category = "Resource Flow")
	int32 ExtractResources(FGuid NodeID, EResourceType ResourceType, int32 MaxAmount);

	/**
	 * Get current resource flow rate through a connection
	 */
	UFUNCTION(BlueprintCallable, Category = "Resource Flow")
	float GetConnectionFlowRate(FGuid ConnectionID) const;

	/**
	 * Get resource buffer contents for a node
	 */
	UFUNCTION(BlueprintCallable, Category = "Resource Flow")
	FResourceBuffer GetNodeInputBuffer(FGuid NodeID) const;

	UFUNCTION(BlueprintCallable, Category = "Resource Flow")
	FResourceBuffer GetNodeOutputBuffer(FGuid NodeID) const;

	// ============================================================================
	// Analysis and Optimization
	// ============================================================================

	/**
	 * Analyze production line for bottlenecks
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	FBottleneckAnalysis AnalyzeBottlenecks(FGuid LineID) const;

	/**
	 * Get optimization recommendations
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	TArray<FString> GetOptimizationRecommendations(FGuid LineID) const;

	/**
	 * Calculate theoretical maximum throughput
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	float CalculateMaxThroughput(FGuid LineID) const;

	/**
	 * Get efficiency report for production line
	 */
	UFUNCTION(BlueprintCallable, Category = "Analysis")
	float GetProductionLineEfficiency(FGuid LineID) const;

	// ============================================================================
	// Statistics
	// ============================================================================

	/**
	 * Get overall network statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	FAutomationNetworkStats GetNetworkStatistics() const;

	/**
	 * Get node performance metrics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	FAutomationNode GetNodeMetrics(FGuid NodeID) const;

	/**
	 * Reset statistics
	 */
	UFUNCTION(BlueprintCallable, Category = "Statistics")
	void ResetStatistics();

	// ============================================================================
	// Integration
	// ============================================================================

	/**
	 * Set crafting manager reference
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetCraftingManager(UOdysseyCraftingManager* NewManager);

	/**
	 * Set inventory component for input/output
	 */
	UFUNCTION(BlueprintCallable, Category = "Integration")
	void SetInventoryComponent(UOdysseyInventoryComponent* NewInventory);

	// ============================================================================
	// Events
	// ============================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnNodeStateChanged OnNodeStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnResourceTransferred OnResourceTransferred;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnProductionCompleted OnProductionCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBottleneckDetected OnBottleneckDetected;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnProductionLineStatusChanged OnProductionLineStatusChanged;

protected:
	// ============================================================================
	// Configuration
	// ============================================================================

	/**
	 * Update frequency for network simulation
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float NetworkUpdateFrequency;

	/**
	 * Maximum nodes allowed in network
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	int32 MaxNodesInNetwork;

	/**
	 * Enable automatic bottleneck detection
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bAutoDetectBottlenecks;

	/**
	 * Bottleneck detection interval
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	float BottleneckDetectionInterval;

	// ============================================================================
	// Runtime State
	// ============================================================================

	/**
	 * All automation nodes
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FGuid, FAutomationNode> Nodes;

	/**
	 * All connections
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FGuid, FAutomationConnection> Connections;

	/**
	 * Production lines
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TMap<FGuid, FProductionLine> ProductionLines;

	/**
	 * Network statistics
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FAutomationNetworkStats Statistics;

	/**
	 * Time accumulators
	 */
	float TimeSinceLastUpdate;
	float TimeSinceLastBottleneckCheck;

	// ============================================================================
	// Component References
	// ============================================================================

	UPROPERTY()
	UOdysseyCraftingManager* CraftingManager;

	UPROPERTY()
	UOdysseyInventoryComponent* InventoryComponent;

	// ============================================================================
	// Internal Methods
	// ============================================================================

	/**
	 * Process all nodes in the network
	 */
	void ProcessNetwork(float DeltaTime);

	/**
	 * Process a single node
	 */
	void ProcessNode(FAutomationNode& Node, float DeltaTime);

	/**
	 * Process resource flow through connections
	 */
	void ProcessResourceFlow(float DeltaTime);

	/**
	 * Transfer resources through a connection
	 */
	void TransferResources(FAutomationConnection& Connection, float DeltaTime);

	/**
	 * Update node state based on buffers and connections
	 */
	void UpdateNodeState(FAutomationNode& Node);

	/**
	 * Update production line metrics
	 */
	void UpdateProductionLineMetrics(FProductionLine& Line);

	/**
	 * Check for bottlenecks in active lines
	 */
	void CheckForBottlenecks();

	/**
	 * Update network statistics
	 */
	void UpdateStatistics();

	/**
	 * Get upstream nodes (nodes that feed into this one)
	 */
	TArray<FGuid> GetUpstreamNodes(FGuid NodeID) const;

	/**
	 * Get downstream nodes (nodes that this one feeds into)
	 */
	TArray<FGuid> GetDownstreamNodes(FGuid NodeID) const;

	/**
	 * Check for cycles in the graph
	 */
	bool HasCycle(FGuid StartNodeID, FGuid EndNodeID) const;

	/**
	 * Generate default node configuration
	 */
	FAutomationNode CreateDefaultNode(EAutomationNodeType NodeType) const;
};
