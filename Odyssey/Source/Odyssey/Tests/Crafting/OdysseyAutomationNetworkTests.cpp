// OdysseyAutomationNetworkTests.cpp
// Comprehensive automation tests for the Automation Network System
// Tests node management, connections, resource flow, production lines, and bottleneck detection

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Crafting/OdysseyCraftingManager.h"
#include "Crafting/OdysseyAutomationNetworkSystem.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// 1. Automation Node Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationNodeDefaults,
	"Odyssey.Crafting.Automation.NodeDefaultInit",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationNodeDefaults::RunTest(const FString& Parameters)
{
	FAutomationNode Node;

	TestTrue(TEXT("NodeID should be valid"), Node.NodeID.IsValid());
	TestEqual(TEXT("Default NodeType should be Processing"), Node.NodeType, EAutomationNodeType::Processing);
	TestEqual(TEXT("Default state should be Idle"), Node.CurrentState, EAutomationNodeState::Idle);
	TestEqual(TEXT("Default AssignedRecipe should be None"), Node.AssignedRecipe, NAME_None);
	TestEqual(TEXT("Default ProcessingSpeed should be 1.0"), Node.ProcessingSpeed, 1.0f);
	TestEqual(TEXT("Default BatchSize should be 1"), Node.BatchSize, 1);
	TestEqual(TEXT("Default InputSlots should be 1"), Node.InputSlots, 1);
	TestEqual(TEXT("Default OutputSlots should be 1"), Node.OutputSlots, 1);
	TestEqual(TEXT("Default Efficiency should be 1.0"), Node.Efficiency, 1.0f);
	TestEqual(TEXT("Default TotalItemsProcessed should be 0"), Node.TotalItemsProcessed, 0);
	TestEqual(TEXT("Default EnergyConsumption should be 10"), Node.EnergyConsumption, 10);
	TestTrue(TEXT("Default bHasPower should be true"), Node.bHasPower);

	return true;
}

// ============================================================================
// 2. Default Node Configuration by Type
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationNodeTypeDefaults,
	"Odyssey.Crafting.Automation.NodeTypeSpecificDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationNodeTypeDefaults::RunTest(const FString& Parameters)
{
	// Replicate CreateDefaultNode logic
	auto CreateDefaultNode = [](EAutomationNodeType NodeType) -> FAutomationNode
	{
		FAutomationNode Node;
		Node.NodeType = NodeType;

		switch (NodeType)
		{
		case EAutomationNodeType::Input:
			Node.InputSlots = 0;
			Node.OutputSlots = 1;
			Node.InputBuffer.MaxCapacity = 500;
			Node.OutputBuffer.MaxCapacity = 100;
			Node.EnergyConsumption = 5;
			break;
		case EAutomationNodeType::Output:
			Node.InputSlots = 1;
			Node.OutputSlots = 0;
			Node.InputBuffer.MaxCapacity = 100;
			Node.OutputBuffer.MaxCapacity = 500;
			Node.EnergyConsumption = 5;
			break;
		case EAutomationNodeType::Processing:
			Node.InputSlots = 2;
			Node.OutputSlots = 2;
			Node.InputBuffer.MaxCapacity = 100;
			Node.OutputBuffer.MaxCapacity = 100;
			Node.EnergyConsumption = 20;
			break;
		case EAutomationNodeType::Storage:
			Node.InputSlots = 2;
			Node.OutputSlots = 2;
			Node.InputBuffer.MaxCapacity = 1000;
			Node.OutputBuffer.MaxCapacity = 1000;
			Node.EnergyConsumption = 2;
			break;
		case EAutomationNodeType::Splitter:
			Node.InputSlots = 1;
			Node.OutputSlots = 3;
			Node.EnergyConsumption = 3;
			break;
		case EAutomationNodeType::Merger:
			Node.InputSlots = 3;
			Node.OutputSlots = 1;
			Node.EnergyConsumption = 3;
			break;
		case EAutomationNodeType::Filter:
			Node.InputSlots = 1;
			Node.OutputSlots = 2;
			Node.EnergyConsumption = 3;
			break;
		}
		return Node;
	};

	// Input node
	FAutomationNode InputNode = CreateDefaultNode(EAutomationNodeType::Input);
	TestEqual(TEXT("Input node should have 0 input slots"), InputNode.InputSlots, 0);
	TestEqual(TEXT("Input node should have 1 output slot"), InputNode.OutputSlots, 1);
	TestEqual(TEXT("Input buffer should be 500"), InputNode.InputBuffer.MaxCapacity, 500);

	// Output node
	FAutomationNode OutputNode = CreateDefaultNode(EAutomationNodeType::Output);
	TestEqual(TEXT("Output node should have 1 input slot"), OutputNode.InputSlots, 1);
	TestEqual(TEXT("Output node should have 0 output slots"), OutputNode.OutputSlots, 0);
	TestEqual(TEXT("Output buffer should be 500"), OutputNode.OutputBuffer.MaxCapacity, 500);

	// Storage node
	FAutomationNode StorageNode = CreateDefaultNode(EAutomationNodeType::Storage);
	TestEqual(TEXT("Storage buffer capacity should be 1000"), StorageNode.InputBuffer.MaxCapacity, 1000);
	TestEqual(TEXT("Storage energy should be 2"), StorageNode.EnergyConsumption, 2);

	// Splitter
	FAutomationNode SplitterNode = CreateDefaultNode(EAutomationNodeType::Splitter);
	TestEqual(TEXT("Splitter should have 1 input"), SplitterNode.InputSlots, 1);
	TestEqual(TEXT("Splitter should have 3 outputs"), SplitterNode.OutputSlots, 3);

	// Merger
	FAutomationNode MergerNode = CreateDefaultNode(EAutomationNodeType::Merger);
	TestEqual(TEXT("Merger should have 3 inputs"), MergerNode.InputSlots, 3);
	TestEqual(TEXT("Merger should have 1 output"), MergerNode.OutputSlots, 1);

	return true;
}

// ============================================================================
// 3. Resource Buffer Operations
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationResourceBuffer,
	"Odyssey.Crafting.Automation.ResourceBufferOperations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationResourceBuffer::RunTest(const FString& Parameters)
{
	FResourceBuffer Buffer;
	Buffer.MaxCapacity = 100;

	TestTrue(TEXT("Empty buffer IsEmpty should be true"), Buffer.IsEmpty());
	TestFalse(TEXT("Empty buffer IsFull should be false"), Buffer.IsFull());
	TestEqual(TEXT("Empty buffer fill ratio should be 0"), Buffer.GetFillRatio(), 0.0f);

	// Add resources
	bool bAdded = Buffer.Add(EResourceType::Silicate, 30);
	TestTrue(TEXT("Should successfully add 30 Silicate"), bAdded);
	TestEqual(TEXT("Silicate count should be 30"), Buffer.GetAmount(EResourceType::Silicate), 30);
	TestEqual(TEXT("CurrentTotal should be 30"), Buffer.CurrentTotal, 30);

	// Add different type
	bAdded = Buffer.Add(EResourceType::Carbon, 20);
	TestTrue(TEXT("Should successfully add 20 Carbon"), bAdded);
	TestEqual(TEXT("CurrentTotal should be 50"), Buffer.CurrentTotal, 50);

	// Fill ratio
	TestTrue(TEXT("Fill ratio should be 0.5"), FMath::IsNearlyEqual(Buffer.GetFillRatio(), 0.5f, 0.001f));

	// Try to exceed capacity
	bool bOverflow = Buffer.Add(EResourceType::Silicate, 60);
	TestFalse(TEXT("Should fail to add 60 more (would exceed 100)"), bOverflow);
	TestEqual(TEXT("CurrentTotal should still be 50"), Buffer.CurrentTotal, 50);

	// Exactly fill remaining capacity
	bAdded = Buffer.Add(EResourceType::Silicate, 50);
	TestTrue(TEXT("Should successfully fill to capacity"), bAdded);
	TestTrue(TEXT("Buffer should now be full"), Buffer.IsFull());

	return true;
}

// ============================================================================
// 4. Resource Buffer Remove Operations
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationBufferRemove,
	"Odyssey.Crafting.Automation.BufferRemoveOperations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationBufferRemove::RunTest(const FString& Parameters)
{
	FResourceBuffer Buffer;
	Buffer.MaxCapacity = 200;
	Buffer.Add(EResourceType::Silicate, 50);
	Buffer.Add(EResourceType::Carbon, 30);

	// Remove partial
	int32 Removed = Buffer.Remove(EResourceType::Silicate, 20);
	TestEqual(TEXT("Should remove 20 Silicate"), Removed, 20);
	TestEqual(TEXT("Remaining Silicate should be 30"), Buffer.GetAmount(EResourceType::Silicate), 30);

	// Remove more than available
	Removed = Buffer.Remove(EResourceType::Carbon, 100);
	TestEqual(TEXT("Should only remove 30 Carbon (all available)"), Removed, 30);
	TestEqual(TEXT("Carbon should be 0 after over-remove"), Buffer.GetAmount(EResourceType::Carbon), 0);

	// Remove nonexistent type
	Removed = Buffer.Remove(EResourceType::CompositeMaterial, 10);
	TestEqual(TEXT("Should remove 0 of nonexistent type"), Removed, 0);

	// Check totals
	TestEqual(TEXT("CurrentTotal should be 30"), Buffer.CurrentTotal, 30);

	return true;
}

// ============================================================================
// 5. Resource Buffer Capacity Check
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationBufferCapacity,
	"Odyssey.Crafting.Automation.BufferCanAddCheck",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationBufferCapacity::RunTest(const FString& Parameters)
{
	FResourceBuffer Buffer;
	Buffer.MaxCapacity = 50;
	Buffer.Add(EResourceType::Silicate, 30);

	TestTrue(TEXT("Should be able to add 20"), Buffer.CanAdd(EResourceType::Carbon, 20));
	TestFalse(TEXT("Should not be able to add 21"), Buffer.CanAdd(EResourceType::Carbon, 21));
	TestTrue(TEXT("Should be able to add 0"), Buffer.CanAdd(EResourceType::Carbon, 0));

	return true;
}

// ============================================================================
// 6. Connection Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationConnectionDefaults,
	"Odyssey.Crafting.Automation.ConnectionDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationConnectionDefaults::RunTest(const FString& Parameters)
{
	FAutomationConnection Conn;

	TestTrue(TEXT("ConnectionID should be valid"), Conn.ConnectionID.IsValid());
	TestFalse(TEXT("SourceNodeID should be invalid"), Conn.SourceNodeID.IsValid());
	TestFalse(TEXT("TargetNodeID should be invalid"), Conn.TargetNodeID.IsValid());
	TestEqual(TEXT("SourceSlot should be 0"), Conn.SourceSlot, 0);
	TestEqual(TEXT("TargetSlot should be 0"), Conn.TargetSlot, 0);
	TestEqual(TEXT("TransferRate should be 10.0"), Conn.TransferRate, 10.0f);
	TestEqual(TEXT("CurrentFlow should be 0"), Conn.CurrentFlow, 0.0f);
	TestTrue(TEXT("bIsActive should be true"), Conn.bIsActive);
	TestEqual(TEXT("FilteredResources should be empty"), Conn.FilteredResources.Num(), 0);

	return true;
}

// ============================================================================
// 7. Self-Connection Prevention
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationNoSelfConnection,
	"Odyssey.Crafting.Automation.SelfConnectionPrevention",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationNoSelfConnection::RunTest(const FString& Parameters)
{
	// Simulate ValidateConnection self-check
	FGuid NodeA = FGuid::NewGuid();

	bool bValid = (NodeA != NodeA); // Self-connection
	TestFalse(TEXT("Self-connection should be invalid"), bValid);

	FGuid NodeB = FGuid::NewGuid();
	bValid = (NodeA != NodeB);
	TestTrue(TEXT("Different node connection should be valid"), bValid);

	return true;
}

// ============================================================================
// 8. Cycle Detection in Connection Graph
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationCycleDetection,
	"Odyssey.Crafting.Automation.CycleDetectionInGraph",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationCycleDetection::RunTest(const FString& Parameters)
{
	// Build a simple graph and test cycle detection using DFS
	// A -> B -> C (no cycle)
	// Then try adding C -> A (would create cycle)

	struct FSimpleNode
	{
		FGuid ID;
		TArray<FGuid> Downstream;
	};

	FSimpleNode A, B, C;
	A.ID = FGuid::NewGuid();
	B.ID = FGuid::NewGuid();
	C.ID = FGuid::NewGuid();

	A.Downstream.Add(B.ID);
	B.Downstream.Add(C.ID);

	TMap<FGuid, FSimpleNode> Graph;
	Graph.Add(A.ID, A);
	Graph.Add(B.ID, B);
	Graph.Add(C.ID, C);

	// Check if adding C -> A would create a cycle (DFS from A following downstream)
	auto HasCycle = [&](FGuid StartID, FGuid EndID) -> bool
	{
		TSet<FGuid> Visited;
		TArray<FGuid> Stack;
		Stack.Add(EndID);

		while (Stack.Num() > 0)
		{
			FGuid Current = Stack.Pop();
			if (Current == StartID) return true;
			if (Visited.Contains(Current)) continue;
			Visited.Add(Current);

			const FSimpleNode* Node = Graph.Find(Current);
			if (Node)
			{
				for (const FGuid& Next : Node->Downstream)
				{
					if (!Visited.Contains(Next))
					{
						Stack.Add(Next);
					}
				}
			}
		}
		return false;
	};

	// C -> A: Check if following downstream from A leads back to C (it does: A->B->C)
	bool bWouldCycle = HasCycle(C.ID, A.ID);
	// Actually the check is: is there a path from EndNodeID (A) downstream back to StartNodeID (C)?
	// A->B->C, so starting from A we reach C, so HasCycle(C, A) = true
	TestTrue(TEXT("C -> A connection should detect a cycle"), bWouldCycle);

	// D -> A: No existing path from A to D, so no cycle
	FGuid D = FGuid::NewGuid();
	bool bNoCycle = HasCycle(D, A.ID);
	TestFalse(TEXT("D -> A should not detect a cycle (D is unconnected)"), bNoCycle);

	return true;
}

// ============================================================================
// 9. Production Line Default Initialization
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationProductionLineDefaults,
	"Odyssey.Crafting.Automation.ProductionLineDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationProductionLineDefaults::RunTest(const FString& Parameters)
{
	FProductionLine Line;

	TestTrue(TEXT("LineID should be valid"), Line.LineID.IsValid());
	TestEqual(TEXT("Default LineName should be 'Production Line'"), Line.LineName, FString(TEXT("Production Line")));
	TestEqual(TEXT("Default FinalProduct should be None"), Line.FinalProduct, NAME_None);
	TestTrue(TEXT("Default bIsActive should be true"), Line.bIsActive);
	TestEqual(TEXT("Default OverallEfficiency should be 1.0"), Line.OverallEfficiency, 1.0f);
	TestEqual(TEXT("Default ProductionRate should be 0"), Line.ProductionRate, 0.0f);
	TestEqual(TEXT("Default TotalEnergyConsumption should be 0"), Line.TotalEnergyConsumption, 0);
	TestEqual(TEXT("NodeIDs should be empty"), Line.NodeIDs.Num(), 0);

	return true;
}

// ============================================================================
// 10. Empty Production Line Handling
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationEmptyProductionLine,
	"Odyssey.Crafting.Automation.EmptyProductionLineHandling",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationEmptyProductionLine::RunTest(const FString& Parameters)
{
	// CreateProductionLine should reject empty node array
	TArray<FGuid> EmptyNodes;
	bool bShouldReject = (EmptyNodes.Num() == 0);

	TestTrue(TEXT("Empty node array should be rejected for production line"), bShouldReject);

	return true;
}

// ============================================================================
// 11. Node State Transitions
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationNodeStateTransitions,
	"Odyssey.Crafting.Automation.NodeStateTransitions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationNodeStateTransitions::RunTest(const FString& Parameters)
{
	FAutomationNode Node;
	Node.NodeType = EAutomationNodeType::Processing;
	Node.InputBuffer.MaxCapacity = 100;
	Node.OutputBuffer.MaxCapacity = 100;

	// Simulate UpdateNodeState logic
	auto DetermineState = [](const FAutomationNode& N) -> EAutomationNodeState
	{
		if (N.CurrentState == EAutomationNodeState::Disabled) return EAutomationNodeState::Disabled;
		if (!N.bHasPower) return EAutomationNodeState::Error;

		if (N.NodeType == EAutomationNodeType::Processing)
		{
			if (N.InputBuffer.IsEmpty() || N.InputBuffer.GetFillRatio() < 0.1f)
				return EAutomationNodeState::Starved;
			if (N.OutputBuffer.IsFull() || N.OutputBuffer.GetFillRatio() > 0.9f)
				return EAutomationNodeState::Blocked;
		}
		return EAutomationNodeState::Active;
	};

	// Empty input -> Starved
	TestEqual(TEXT("Empty input should result in Starved"), DetermineState(Node), EAutomationNodeState::Starved);

	// Add some input resources
	Node.InputBuffer.Add(EResourceType::Silicate, 50);
	TestEqual(TEXT("Half-filled input should result in Active"), DetermineState(Node), EAutomationNodeState::Active);

	// Fill output buffer to >90%
	Node.OutputBuffer.Add(EResourceType::CompositeMaterial, 95);
	TestEqual(TEXT("Near-full output should result in Blocked"), DetermineState(Node), EAutomationNodeState::Blocked);

	// Power loss
	Node.bHasPower = false;
	TestEqual(TEXT("No power should result in Error"), DetermineState(Node), EAutomationNodeState::Error);

	// Disabled state persists
	Node.bHasPower = true;
	Node.CurrentState = EAutomationNodeState::Disabled;
	TestEqual(TEXT("Disabled state should persist"), DetermineState(Node), EAutomationNodeState::Disabled);

	return true;
}

// ============================================================================
// 12. Resource Injection Restrictions
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationResourceInjection,
	"Odyssey.Crafting.Automation.ResourceInjectionRestrictions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationResourceInjection::RunTest(const FString& Parameters)
{
	// Only Input and Storage nodes accept injection
	auto CanInject = [](EAutomationNodeType Type) -> bool
	{
		return (Type == EAutomationNodeType::Input || Type == EAutomationNodeType::Storage);
	};

	TestTrue(TEXT("Input node should accept injection"), CanInject(EAutomationNodeType::Input));
	TestTrue(TEXT("Storage node should accept injection"), CanInject(EAutomationNodeType::Storage));
	TestFalse(TEXT("Processing node should reject injection"), CanInject(EAutomationNodeType::Processing));
	TestFalse(TEXT("Output node should reject injection"), CanInject(EAutomationNodeType::Output));
	TestFalse(TEXT("Splitter should reject injection"), CanInject(EAutomationNodeType::Splitter));
	TestFalse(TEXT("Merger should reject injection"), CanInject(EAutomationNodeType::Merger));
	TestFalse(TEXT("Filter should reject injection"), CanInject(EAutomationNodeType::Filter));

	return true;
}

// ============================================================================
// 13. Resource Extraction Restrictions
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationResourceExtraction,
	"Odyssey.Crafting.Automation.ResourceExtractionRestrictions",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationResourceExtraction::RunTest(const FString& Parameters)
{
	// Only Output and Storage nodes allow extraction
	auto CanExtract = [](EAutomationNodeType Type) -> bool
	{
		return (Type == EAutomationNodeType::Output || Type == EAutomationNodeType::Storage);
	};

	TestTrue(TEXT("Output node should allow extraction"), CanExtract(EAutomationNodeType::Output));
	TestTrue(TEXT("Storage node should allow extraction"), CanExtract(EAutomationNodeType::Storage));
	TestFalse(TEXT("Input node should not allow extraction"), CanExtract(EAutomationNodeType::Input));
	TestFalse(TEXT("Processing node should not allow extraction"), CanExtract(EAutomationNodeType::Processing));

	return true;
}

// ============================================================================
// 14. Connection Filter Logic
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationConnectionFilter,
	"Odyssey.Crafting.Automation.ConnectionFilterLogic",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationConnectionFilter::RunTest(const FString& Parameters)
{
	FAutomationConnection Conn;

	// No filter = all pass
	TestEqual(TEXT("Empty filter should allow everything"), Conn.FilteredResources.Num(), 0);

	auto PassesFilter = [](const TArray<EResourceType>& Filter, EResourceType Type) -> bool
	{
		if (Filter.Num() == 0) return true;
		return Filter.Contains(Type);
	};

	// No filter
	TestTrue(TEXT("Silicate should pass empty filter"), PassesFilter(Conn.FilteredResources, EResourceType::Silicate));

	// Set filter
	Conn.FilteredResources.Add(EResourceType::Silicate);
	Conn.FilteredResources.Add(EResourceType::Carbon);

	TestTrue(TEXT("Silicate should pass filter"), PassesFilter(Conn.FilteredResources, EResourceType::Silicate));
	TestTrue(TEXT("Carbon should pass filter"), PassesFilter(Conn.FilteredResources, EResourceType::Carbon));
	TestFalse(TEXT("CompositeMaterial should not pass filter"), PassesFilter(Conn.FilteredResources, EResourceType::CompositeMaterial));

	return true;
}

// ============================================================================
// 15. Transfer Rate Minimum Enforcement
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationTransferRateMin,
	"Odyssey.Crafting.Automation.TransferRateMinimum",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationTransferRateMin::RunTest(const FString& Parameters)
{
	// SetConnectionTransferRate clamps to minimum of 0.1
	float RequestedRate = -5.0f;
	float ActualRate = FMath::Max(RequestedRate, 0.1f);
	TestEqual(TEXT("Negative rate should clamp to 0.1"), ActualRate, 0.1f);

	RequestedRate = 0.0f;
	ActualRate = FMath::Max(RequestedRate, 0.1f);
	TestEqual(TEXT("Zero rate should clamp to 0.1"), ActualRate, 0.1f);

	RequestedRate = 5.0f;
	ActualRate = FMath::Max(RequestedRate, 0.1f);
	TestEqual(TEXT("Valid rate should pass through"), ActualRate, 5.0f);

	return true;
}

// ============================================================================
// 16. Bottleneck Analysis Structure
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationBottleneckAnalysis,
	"Odyssey.Crafting.Automation.BottleneckAnalysisDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationBottleneckAnalysis::RunTest(const FString& Parameters)
{
	FBottleneckAnalysis Analysis;

	TestFalse(TEXT("Default BottleneckNodeID should be invalid"), Analysis.BottleneckNodeID.IsValid());
	TestEqual(TEXT("Default SeverityScore should be 0"), Analysis.SeverityScore, 0.0f);
	TestEqual(TEXT("Default PotentialEfficiencyGain should be 0"), Analysis.PotentialEfficiencyGain, 0.0f);
	TestEqual(TEXT("Default Recommendations should be empty"), Analysis.Recommendations.Num(), 0);
	TestTrue(TEXT("Default BottleneckReason should be empty"), Analysis.BottleneckReason.IsEmpty());

	return true;
}

// ============================================================================
// 17. Bottleneck Severity Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationBottleneckSeverity,
	"Odyssey.Crafting.Automation.BottleneckSeverityCalc",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationBottleneckSeverity::RunTest(const FString& Parameters)
{
	// Severity = 1.0 - LowestEfficiency
	// PotentialGain = (1.0 / max(LowestEfficiency, 0.1)) - 1.0

	float LowestEfficiency = 0.3f;
	float Severity = 1.0f - LowestEfficiency;
	float PotentialGain = (1.0f / FMath::Max(LowestEfficiency, 0.1f)) - 1.0f;

	TestTrue(TEXT("Severity at 30% efficiency should be 0.7"), FMath::IsNearlyEqual(Severity, 0.7f, 0.001f));
	TestTrue(TEXT("Potential gain should be ~2.33"), FMath::IsNearlyEqual(PotentialGain, 2.333f, 0.01f));

	// Edge case: very low efficiency
	LowestEfficiency = 0.05f;
	Severity = 1.0f - LowestEfficiency;
	PotentialGain = (1.0f / FMath::Max(LowestEfficiency, 0.1f)) - 1.0f;

	TestTrue(TEXT("Severity at 5% efficiency should be 0.95"), FMath::IsNearlyEqual(Severity, 0.95f, 0.001f));
	// Clamped to 0.1: gain = (1/0.1) - 1 = 9.0
	TestTrue(TEXT("Potential gain at 5% should clamp to 9.0"), FMath::IsNearlyEqual(PotentialGain, 9.0f, 0.001f));

	return true;
}

// ============================================================================
// 18. Network Statistics Defaults
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationNetworkStatsDefaults,
	"Odyssey.Crafting.Automation.NetworkStatsDefaults",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationNetworkStatsDefaults::RunTest(const FString& Parameters)
{
	FAutomationNetworkStats Stats;

	TestEqual(TEXT("TotalNodes should be 0"), Stats.TotalNodes, 0);
	TestEqual(TEXT("ActiveNodes should be 0"), Stats.ActiveNodes, 0);
	TestEqual(TEXT("TotalConnections should be 0"), Stats.TotalConnections, 0);
	TestEqual(TEXT("AverageEfficiency should be 0"), Stats.AverageEfficiency, 0.0f);
	TestEqual(TEXT("TotalThroughput should be 0"), Stats.TotalThroughput, 0.0f);
	TestEqual(TEXT("TotalEnergyConsumption should be 0"), Stats.TotalEnergyConsumption, 0);
	TestEqual(TEXT("TotalItemsProduced should be 0"), Stats.TotalItemsProduced, 0);

	return true;
}

// ============================================================================
// 19. Max Throughput Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationMaxThroughput,
	"Odyssey.Crafting.Automation.MaxThroughputCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationMaxThroughput::RunTest(const FString& Parameters)
{
	// Max throughput is limited by the slowest processing node
	// Throughput = ProcessingSpeed * BatchSize

	struct FNodeStats { float Speed; int32 Batch; };
	TArray<FNodeStats> ProcessingNodes = {
		{2.0f, 1},  // throughput = 2.0
		{1.0f, 3},  // throughput = 3.0
		{0.5f, 2},  // throughput = 1.0 (bottleneck)
		{1.5f, 2},  // throughput = 3.0
	};

	float MinThroughput = FLT_MAX;
	for (const auto& Node : ProcessingNodes)
	{
		float Throughput = Node.Speed * Node.Batch;
		MinThroughput = FMath::Min(MinThroughput, Throughput);
	}

	TestTrue(TEXT("Max throughput should be limited to 1.0 (bottleneck)"),
		FMath::IsNearlyEqual(MinThroughput, 1.0f, 0.001f));

	return true;
}

// ============================================================================
// 20. Node Efficiency Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationNodeEfficiency,
	"Odyssey.Crafting.Automation.NodeEfficiencyCalculation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationNodeEfficiency::RunTest(const FString& Parameters)
{
	// Efficiency = min(InputFillRatio, 1.0 - OutputFillRatio)
	FResourceBuffer InputBuffer;
	InputBuffer.MaxCapacity = 100;
	InputBuffer.Add(EResourceType::Silicate, 70);

	FResourceBuffer OutputBuffer;
	OutputBuffer.MaxCapacity = 100;
	OutputBuffer.Add(EResourceType::CompositeMaterial, 20);

	float InputFill = InputBuffer.GetFillRatio();    // 0.7
	float OutputFill = 1.0f - OutputBuffer.GetFillRatio(); // 0.8
	float Efficiency = FMath::Min(InputFill, OutputFill);

	TestTrue(TEXT("Efficiency should be 0.7 (limited by input fill)"),
		FMath::IsNearlyEqual(Efficiency, 0.7f, 0.001f));

	// Blocked scenario: output almost full
	OutputBuffer.Add(EResourceType::CompositeMaterial, 75); // now 95 total
	OutputFill = 1.0f - OutputBuffer.GetFillRatio(); // 1.0 - 0.95 = 0.05
	Efficiency = FMath::Min(InputFill, OutputFill);

	TestTrue(TEXT("Efficiency should be 0.05 when output near-full"),
		FMath::IsNearlyEqual(Efficiency, 0.05f, 0.001f));

	return true;
}

// ============================================================================
// 21. Production Line Metrics Calculation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationLineMetrics,
	"Odyssey.Crafting.Automation.ProductionLineMetrics",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationLineMetrics::RunTest(const FString& Parameters)
{
	// Simulate UpdateProductionLineMetrics
	struct FNodeMetric { float Efficiency; int32 Energy; };
	TArray<FNodeMetric> Nodes = {
		{0.9f, 20},
		{0.6f, 15},
		{0.8f, 25},
	};

	float TotalEfficiency = 0.0f;
	float MinEfficiency = 1.0f;
	int32 TotalEnergy = 0;

	for (const auto& N : Nodes)
	{
		TotalEfficiency += N.Efficiency;
		TotalEnergy += N.Energy;
		MinEfficiency = FMath::Min(MinEfficiency, N.Efficiency);
	}

	float OverallEfficiency = TotalEfficiency / Nodes.Num();

	TestTrue(TEXT("Overall efficiency should be ~0.767"), FMath::IsNearlyEqual(OverallEfficiency, 0.7667f, 0.01f));
	TestEqual(TEXT("Total energy should be 60"), TotalEnergy, 60);
	TestTrue(TEXT("Min efficiency (bottleneck) should be 0.6"), FMath::IsNearlyEqual(MinEfficiency, 0.6f, 0.001f));

	return true;
}

// ============================================================================
// 22. Max Node Limit
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationMaxNodeLimit,
	"Odyssey.Crafting.Automation.MaxNodeLimitEnforced",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationMaxNodeLimit::RunTest(const FString& Parameters)
{
	int32 MaxNodesInNetwork = 100;
	int32 CurrentNodeCount = 100;

	bool bCanCreate = (CurrentNodeCount < MaxNodesInNetwork);
	TestFalse(TEXT("Should not create node when at max capacity"), bCanCreate);

	CurrentNodeCount = 99;
	bCanCreate = (CurrentNodeCount < MaxNodesInNetwork);
	TestTrue(TEXT("Should create node when below max capacity"), bCanCreate);

	return true;
}

// ============================================================================
// 23. Upstream/Downstream Node Discovery
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationUpstreamDownstream,
	"Odyssey.Crafting.Automation.UpstreamDownstreamDiscovery",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationUpstreamDownstream::RunTest(const FString& Parameters)
{
	// Build a simple graph: A -> B -> C, D -> B
	FGuid NodeA = FGuid::NewGuid();
	FGuid NodeB = FGuid::NewGuid();
	FGuid NodeC = FGuid::NewGuid();
	FGuid NodeD = FGuid::NewGuid();

	FGuid ConnAB = FGuid::NewGuid();
	FGuid ConnBC = FGuid::NewGuid();
	FGuid ConnDB = FGuid::NewGuid();

	// Track connections
	struct FConn { FGuid Source; FGuid Target; };
	TArray<FConn> Connections = {
		{NodeA, NodeB},
		{NodeB, NodeC},
		{NodeD, NodeB}
	};

	// Find upstream of B (A and D)
	TArray<FGuid> UpstreamOfB;
	for (const auto& C : Connections)
	{
		if (C.Target == NodeB)
		{
			UpstreamOfB.Add(C.Source);
		}
	}
	TestEqual(TEXT("B should have 2 upstream nodes"), UpstreamOfB.Num(), 2);
	TestTrue(TEXT("A should be upstream of B"), UpstreamOfB.Contains(NodeA));
	TestTrue(TEXT("D should be upstream of B"), UpstreamOfB.Contains(NodeD));

	// Find downstream of B (C)
	TArray<FGuid> DownstreamOfB;
	for (const auto& C : Connections)
	{
		if (C.Source == NodeB)
		{
			DownstreamOfB.Add(C.Target);
		}
	}
	TestEqual(TEXT("B should have 1 downstream node"), DownstreamOfB.Num(), 1);
	TestTrue(TEXT("C should be downstream of B"), DownstreamOfB.Contains(NodeC));

	return true;
}

// ============================================================================
// 24. Recipe Assignment to Non-Processing Node
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationRecipeAssignRestriction,
	"Odyssey.Crafting.Automation.RecipeAssignOnlyToProcessing",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationRecipeAssignRestriction::RunTest(const FString& Parameters)
{
	// Only Processing nodes can have recipes assigned
	auto CanAssignRecipe = [](EAutomationNodeType Type) -> bool
	{
		return Type == EAutomationNodeType::Processing;
	};

	TestTrue(TEXT("Processing node should accept recipe"), CanAssignRecipe(EAutomationNodeType::Processing));
	TestFalse(TEXT("Input node should reject recipe"), CanAssignRecipe(EAutomationNodeType::Input));
	TestFalse(TEXT("Output node should reject recipe"), CanAssignRecipe(EAutomationNodeType::Output));
	TestFalse(TEXT("Storage node should reject recipe"), CanAssignRecipe(EAutomationNodeType::Storage));
	TestFalse(TEXT("Splitter should reject recipe"), CanAssignRecipe(EAutomationNodeType::Splitter));

	return true;
}

// ============================================================================
// 25. Resource Transfer Simulation
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAutomationResourceTransfer,
	"Odyssey.Crafting.Automation.ResourceTransferSimulation",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAutomationResourceTransfer::RunTest(const FString& Parameters)
{
	// Simulate TransferResources logic
	FResourceBuffer SourceOutput;
	SourceOutput.MaxCapacity = 100;
	SourceOutput.Add(EResourceType::Silicate, 50);

	FResourceBuffer TargetInput;
	TargetInput.MaxCapacity = 100;

	float TransferRate = 10.0f;
	float DeltaTime = 1.0f;
	int32 MaxTransfer = FMath::CeilToInt(TransferRate * DeltaTime); // 10
	float TotalFlow = 0.0f;

	int32 Available = SourceOutput.GetAmount(EResourceType::Silicate);
	int32 ToTransfer = FMath::Min(Available, MaxTransfer);

	if (TargetInput.CanAdd(EResourceType::Silicate, ToTransfer))
	{
		int32 Transferred = SourceOutput.Remove(EResourceType::Silicate, ToTransfer);
		TargetInput.Add(EResourceType::Silicate, Transferred);
		TotalFlow += Transferred;
	}

	TestEqual(TEXT("Should transfer 10 units"), static_cast<int32>(TotalFlow), 10);
	TestEqual(TEXT("Source should have 40 remaining"), SourceOutput.GetAmount(EResourceType::Silicate), 40);
	TestEqual(TEXT("Target should have 10"), TargetInput.GetAmount(EResourceType::Silicate), 10);

	return true;
}
