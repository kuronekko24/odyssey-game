// OdysseyCombatDemo.cpp

#include "OdysseyCombatDemo.h"
#include "OdysseyCombatManager.h"
#include "OdysseyCombatIntegration.h"
#include "NPCSpawnManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UOdysseyCombatDemo::UOdysseyCombatDemo()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize state
	CombatManager = nullptr;
	CombatIntegration = nullptr;
	NPCSpawnManager = nullptr;
	bDemoActive = false;
	bAutoStartDemo = true;
	bShowDemoInstructions = true;

	// Set up default demo enemy locations (relative to player)
	DemoEnemyLocations.Add(FVector(1000, 0, 0));     // Forward
	DemoEnemyLocations.Add(FVector(800, 600, 0));    // Forward-right
	DemoEnemyLocations.Add(FVector(800, -600, 0));   // Forward-left
	DemoEnemyLocations.Add(FVector(1200, 300, 100)); // Forward-right-up
	DemoEnemyLocations.Add(FVector(1200, -300, 100)); // Forward-left-up
}

void UOdysseyCombatDemo::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartDemo)
	{
		// Delay initialization slightly to ensure all systems are ready
		GetWorld()->GetTimerManager().SetTimer(
			FTimerHandle(),
			this,
			&UOdysseyCombatDemo::InitializeCombatDemo,
			1.0f,
			false
		);
	}
}

// ============================================================================
// Demo Setup
// ============================================================================

void UOdysseyCombatDemo::InitializeCombatDemo()
{
	UE_LOG(LogTemp, Log, TEXT("Initializing Combat Demo..."));

	// Initialize component references
	InitializeReferences();

	// Ensure combat components exist
	EnsureCombatComponents();

	// Setup combat system
	SetupCombatSystem();

	// Start demo scenario
	if (bAutoStartDemo)
	{
		StartDemoScenario();
	}

	UE_LOG(LogTemp, Log, TEXT("Combat Demo initialized successfully"));
}

void UOdysseyCombatDemo::SetupCombatSystem()
{
	if (!CombatManager)
	{
		UE_LOG(LogTemp, Error, TEXT("Combat Demo: No combat manager found"));
		return;
	}

	// Configure combat system for demo
	ConfigureCombatForDemo();

	// Initialize combat system
	CombatManager->InitializeCombatSystem();

	UE_LOG(LogTemp, Log, TEXT("Combat system setup complete"));
}

void UOdysseyCombatDemo::SpawnDemoEnemies()
{
	if (!GetOwner())
	{
		UE_LOG(LogTemp, Error, TEXT("Combat Demo: No owner actor"));
		return;
	}

	FVector PlayerLocation = GetOwner()->GetActorLocation();
	TArray<FVector> SpawnLocations = GetDemoSpawnLocations();

	// For this demo, we'll create simple enemy actors
	// In a full implementation, you'd use the NPC spawn manager
	for (const FVector& RelativeLocation : SpawnLocations)
	{
		FVector SpawnLocation = PlayerLocation + RelativeLocation;
		
		// Spawn a simple enemy actor (this would be replaced with actual NPC spawning)
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// For demo purposes, we'll just log the spawn locations
		// In a real implementation, you'd spawn actual NPC ships here
		UE_LOG(LogTemp, Log, TEXT("Demo Enemy would spawn at: %s"), *SpawnLocation.ToString());
		
		OnEnemySpawned(nullptr); // Would pass actual spawned enemy
	}

	UE_LOG(LogTemp, Log, TEXT("Demo enemies spawned"));
}

void UOdysseyCombatDemo::StartDemoScenario()
{
	if (bDemoActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("Demo already active"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Starting Combat Demo Scenario..."));

	// Enable combat system
	if (CombatManager)
	{
		CombatManager->SetCombatEnabled(true);
	}

	// Spawn demo enemies
	SpawnDemoEnemies();

	// Set demo as active
	bDemoActive = true;

	// Show demo instructions
	if (bShowDemoInstructions)
	{
		UE_LOG(LogTemp, Log, TEXT("=== COMBAT DEMO INSTRUCTIONS ==="));
		UE_LOG(LogTemp, Log, TEXT("1. Touch enemies on screen to target them"));
		UE_LOG(LogTemp, Log, TEXT("2. Use Attack button to fire weapons"));
		UE_LOG(LogTemp, Log, TEXT("3. Red circles indicate targeted enemies"));
		UE_LOG(LogTemp, Log, TEXT("4. Health bars appear above damaged enemies"));
		UE_LOG(LogTemp, Log, TEXT("5. Auto-targeting will engage nearest hostile"));
		UE_LOG(LogTemp, Log, TEXT("================================"));
	}

	OnDemoStarted();
	OnCombatEngaged();
}

void UOdysseyCombatDemo::StopDemo()
{
	if (!bDemoActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Stopping Combat Demo..."));

	// Disable combat system
	if (CombatManager)
	{
		CombatManager->SetCombatEnabled(false);
	}

	// Clean up demo enemies (in a real implementation)
	// NPCSpawnManager->DespawnAllNPCs();

	bDemoActive = false;
	OnDemoEnded();

	UE_LOG(LogTemp, Log, TEXT("Combat Demo stopped"));
}

// ============================================================================
// Private Methods
// ============================================================================

void UOdysseyCombatDemo::InitializeReferences()
{
	if (!GetOwner())
	{
		return;
	}

	// Find combat manager
	CombatManager = GetOwner()->FindComponentByClass<UOdysseyCombatManager>();

	// Find combat integration
	CombatIntegration = GetOwner()->FindComponentByClass<UOdysseyCombatIntegration>();

	// Find NPC spawn manager
	NPCSpawnManager = GetWorld()->SpawnActor<ANPCSpawnManager>();
}

void UOdysseyCombatDemo::EnsureCombatComponents()
{
	if (!GetOwner())
	{
		return;
	}

	// Create combat manager if it doesn't exist
	if (!CombatManager)
	{
		CombatManager = NewObject<UOdysseyCombatManager>(GetOwner(), FName("CombatManager"));
		GetOwner()->AddInstanceComponent(CombatManager);
		CombatManager->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("Created Combat Manager component"));
	}

	// Create combat integration if it doesn't exist
	if (!CombatIntegration)
	{
		CombatIntegration = NewObject<UOdysseyCombatIntegration>(GetOwner(), FName("CombatIntegration"));
		GetOwner()->AddInstanceComponent(CombatIntegration);
		CombatIntegration->RegisterComponent();
		UE_LOG(LogTemp, Log, TEXT("Created Combat Integration component"));
	}
}

void UOdysseyCombatDemo::ConfigureCombatForDemo()
{
	if (!CombatManager)
	{
		return;
	}

	// Set up demo-friendly configuration
	FCombatConfiguration Config;
	Config.bEnableAutoTargeting = true;
	Config.bEnableAutoFiring = false;        // Manual firing for demo
	Config.bShowTargetIndicators = true;
	Config.bShowHealthBars = true;
	Config.bShowDamageNumbers = true;
	Config.TargetingRange = 2500.0f;         // Longer range for demo
	Config.WeaponRange = 2000.0f;

	CombatManager->SetCombatConfiguration(Config);

	UE_LOG(LogTemp, Log, TEXT("Combat system configured for demo"));
}

TArray<FVector> UOdysseyCombatDemo::GetDemoSpawnLocations()
{
	// Return configured demo locations
	// In a more advanced setup, these could be dynamically generated
	return DemoEnemyLocations;
}
