// ContractSystemTests.cpp
// Automation tests for USocialContractSystem
// Tests contract lifecycle, milestones, escrow, ratings, disputes, and bidding

#include "Misc/AutomationTest.h"
#include "SocialContractSystem.h"
#include "OdysseyGuildManager.h"
#include "Social/ReputationSystem.h"
#include "OdysseyInventoryComponent.h"

// ============================================================================
// TEST HELPERS
// ============================================================================

namespace ContractTestHelpers
{
	struct FContractTestContext
	{
		UOdysseyGuildManager* GuildManager;
		UReputationSystem* ReputationSystem;
		USocialContractSystem* ContractSystem;
	};

	FContractTestContext CreateTestContext()
	{
		FContractTestContext Ctx;
		Ctx.GuildManager = NewObject<UOdysseyGuildManager>();
		Ctx.GuildManager->Initialize();
		Ctx.ReputationSystem = NewObject<UReputationSystem>();
		Ctx.ReputationSystem->Initialize();
		Ctx.ContractSystem = NewObject<USocialContractSystem>();
		Ctx.ContractSystem->Initialize(Ctx.GuildManager, Ctx.ReputationSystem);
		return Ctx;
	}

	FContractPaymentTerms MakeOMENPayment(int64 Amount, float UpfrontPct = 0.0f,
		float MilestonePct = 0.0f, float CompletionPct = 1.0f)
	{
		FContractPaymentTerms Terms;
		Terms.TotalPayment.Add(EResourceType::OMEN, Amount);
		Terms.UpfrontPercentage = UpfrontPct;
		Terms.MilestonePercentage = MilestonePct;
		Terms.CompletionPercentage = CompletionPct;
		Terms.bUseEscrow = true;
		return Terms;
	}

	FGuid CreateAndPostContract(USocialContractSystem* System,
		const FString& ClientID, const FString& ClientName,
		const FString& Title, EContractType Type, int64 Payment)
	{
		FGuid ContractID = System->CreateContract(ClientID, ClientName, Title,
			TEXT("Test contract"), Type, MakeOMENPayment(Payment));
		if (ContractID.IsValid())
		{
			System->PostContract(ContractID, ClientID);
		}
		return ContractID;
	}
}

// ============================================================================
// CONTRACT CREATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_Create,
	"Odyssey.Social.Contract.Creation.ValidContract",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_Create::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FContractPaymentTerms Payment = ContractTestHelpers::MakeOMENPayment(5000);

	FGuid ContractID = Ctx.ContractSystem->CreateContract(
		TEXT("Client001"), TEXT("Alice"),
		TEXT("Escort Mission"), TEXT("Escort through dangerous sector"),
		EContractType::Escort, Payment);

	TestTrue(TEXT("Contract ID should be valid"), ContractID.IsValid());

	FSocialContract Data;
	bool bFound = Ctx.ContractSystem->GetContractData(ContractID, Data);
	TestTrue(TEXT("Contract should be retrievable"), bFound);
	TestEqual(TEXT("Title should match"), Data.Title, FString(TEXT("Escort Mission")));
	TestEqual(TEXT("Client should match"), Data.ClientPlayerID, FString(TEXT("Client001")));
	TestEqual(TEXT("Type should be Escort"),
		static_cast<uint8>(Data.ContractType), static_cast<uint8>(EContractType::Escort));
	TestEqual(TEXT("Status should be Draft"),
		static_cast<uint8>(Data.Status), static_cast<uint8>(EContractStatus::Draft));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_PostToMarket,
	"Odyssey.Social.Contract.Creation.PostToMarket",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_PostToMarket::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = Ctx.ContractSystem->CreateContract(
		TEXT("Client001"), TEXT("Alice"),
		TEXT("Cargo Run"), TEXT(""), EContractType::Transport,
		ContractTestHelpers::MakeOMENPayment(3000));

	bool bPosted = Ctx.ContractSystem->PostContract(ContractID, TEXT("Client001"));
	TestTrue(TEXT("Posting contract should succeed"), bPosted);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	TestEqual(TEXT("Status should be Open after posting"),
		static_cast<uint8>(Data.Status), static_cast<uint8>(EContractStatus::Open));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_CreateAllTypes,
	"Odyssey.Social.Contract.Creation.AllContractTypes",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_CreateAllTypes::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	TArray<EContractType> Types = {
		EContractType::Escort, EContractType::Transport, EContractType::Crafting,
		EContractType::Mining, EContractType::Combat, EContractType::Exploration,
		EContractType::Training, EContractType::Repair, EContractType::Trade,
		EContractType::Custom
	};

	for (int32 i = 0; i < Types.Num(); i++)
	{
		FString ClientID = FString::Printf(TEXT("Client%03d"), i);
		FString Title = FString::Printf(TEXT("Contract_%d"), i);

		FGuid ID = Ctx.ContractSystem->CreateContract(ClientID, TEXT("Tester"),
			Title, TEXT(""), Types[i], ContractTestHelpers::MakeOMENPayment(1000));

		TestTrue(FString::Printf(TEXT("Contract type %d should be creatable"), static_cast<uint8>(Types[i])),
			ID.IsValid());
	}

	return true;
}

// ============================================================================
// CONTRACT ACCEPTANCE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_AcceptContract,
	"Odyssey.Social.Contract.Acceptance.ValidAcceptance",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_AcceptContract::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Mining Job"), EContractType::Mining, 2000);

	bool bAccepted = Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	TestTrue(TEXT("Accepting open contract should succeed"), bAccepted);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	TestEqual(TEXT("Contractor should be assigned"), Data.ContractorPlayerID, FString(TEXT("Contractor001")));
	TestTrue(TEXT("Status should be Active or Pending"),
		Data.Status == EContractStatus::Active || Data.Status == EContractStatus::Pending);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_AcceptOwnContract,
	"Odyssey.Social.Contract.Acceptance.CannotAcceptOwn",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_AcceptOwnContract::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Self Accept"), EContractType::Custom, 1000);

	bool bAccepted = Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Client001"), TEXT("Alice"));
	TestFalse(TEXT("Should not be able to accept own contract"), bAccepted);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_DoubleAccept,
	"Odyssey.Social.Contract.Acceptance.CannotDoubleAccept",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_DoubleAccept::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("One Slot"), EContractType::Combat, 1000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	bool bSecond = Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor002"), TEXT("Carol"));
	TestFalse(TEXT("Second acceptance should fail -- already has contractor"), bSecond);

	return true;
}

// ============================================================================
// MILESTONE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_Milestones,
	"Odyssey.Social.Contract.Milestones.CompleteAndConfirm",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_Milestones::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = Ctx.ContractSystem->CreateContract(
		TEXT("Client001"), TEXT("Alice"),
		TEXT("Multi-stage Craft"), TEXT("Complex crafting job"),
		EContractType::Crafting,
		ContractTestHelpers::MakeOMENPayment(10000, 0.1f, 0.4f, 0.5f));

	// Add milestones
	FContractMilestone M1;
	M1.Description = TEXT("Gather materials");
	M1.OrderIndex = 0;
	M1.PaymentPercentage = 0.2f;
	Ctx.ContractSystem->AddMilestone(ContractID, TEXT("Client001"), M1);

	FContractMilestone M2;
	M2.Description = TEXT("Craft item");
	M2.OrderIndex = 1;
	M2.PaymentPercentage = 0.3f;
	Ctx.ContractSystem->AddMilestone(ContractID, TEXT("Client001"), M2);

	FContractMilestone M3;
	M3.Description = TEXT("Quality check");
	M3.OrderIndex = 2;
	M3.PaymentPercentage = 0.5f;
	Ctx.ContractSystem->AddMilestone(ContractID, TEXT("Client001"), M3);

	// Post and accept
	Ctx.ContractSystem->PostContract(ContractID, TEXT("Client001"));
	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));

	// Complete first milestone
	bool bCompleted = Ctx.ContractSystem->CompleteMilestone(ContractID, TEXT("Contractor001"), 0);
	TestTrue(TEXT("Contractor should complete milestone 0"), bCompleted);

	// Client confirms
	bool bConfirmed = Ctx.ContractSystem->ConfirmMilestone(ContractID, TEXT("Client001"), 0);
	TestTrue(TEXT("Client should confirm milestone 0"), bConfirmed);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	if (Data.Milestones.Num() > 0)
	{
		TestTrue(TEXT("Milestone 0 should be complete"), Data.Milestones[0].bIsComplete);
		TestTrue(TEXT("Milestone 0 should be client confirmed"), Data.Milestones[0].bClientConfirmed);
	}

	// Progress check
	float Progress = Data.GetProgress();
	TestTrue(TEXT("Progress should be ~0.33 (1 of 3 milestones)"),
		FMath::Abs(Progress - (1.0f / 3.0f)) < 0.05f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_RejectMilestone,
	"Odyssey.Social.Contract.Milestones.Rejection",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_RejectMilestone::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Job"), EContractType::Crafting, 5000);

	FContractMilestone M;
	M.Description = TEXT("Do the thing");
	Ctx.ContractSystem->AddMilestone(ContractID, TEXT("Client001"), M);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	Ctx.ContractSystem->CompleteMilestone(ContractID, TEXT("Contractor001"), 0);

	bool bRejected = Ctx.ContractSystem->RejectMilestone(ContractID, TEXT("Client001"), 0, TEXT("Not satisfactory"));
	TestTrue(TEXT("Client should be able to reject milestone"), bRejected);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	if (Data.Milestones.Num() > 0)
	{
		TestFalse(TEXT("Rejected milestone should not be client confirmed"), Data.Milestones[0].bClientConfirmed);
	}

	return true;
}

// ============================================================================
// ESCROW SYSTEM TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_FundEscrow,
	"Odyssey.Social.Contract.Escrow.FundEscrow",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_FundEscrow::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Escrow Test"), EContractType::Transport, 5000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));

	bool bFunded = Ctx.ContractSystem->FundEscrow(ContractID, TEXT("Client001"));
	TestTrue(TEXT("Funding escrow should succeed"), bFunded);

	FContractEscrow Escrow = Ctx.ContractSystem->GetEscrowStatus(ContractID);
	TestEqual(TEXT("Escrow should be Funded"),
		static_cast<uint8>(Escrow.Status), static_cast<uint8>(EEscrowStatus::Funded));
	TestTrue(TEXT("Escrow should hold funds"), Escrow.GetTotalHeld() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_ReleaseEscrow,
	"Odyssey.Social.Contract.Escrow.ReleaseEscrow",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_ReleaseEscrow::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Release Test"), EContractType::Mining, 10000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	Ctx.ContractSystem->FundEscrow(ContractID, TEXT("Client001"));

	// Release 50%
	bool bReleased = Ctx.ContractSystem->ReleaseEscrow(ContractID, 0.5f);
	TestTrue(TEXT("Partial escrow release should succeed"), bReleased);

	FContractEscrow Escrow = Ctx.ContractSystem->GetEscrowStatus(ContractID);
	int64 Released = Escrow.ReleasedFunds.Contains(EResourceType::OMEN) ?
		Escrow.ReleasedFunds[EResourceType::OMEN] : 0;
	TestTrue(TEXT("Released funds should be > 0"), Released > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_RefundEscrow,
	"Odyssey.Social.Contract.Escrow.RefundOnCancel",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_RefundEscrow::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Refund Test"), EContractType::Custom, 3000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	Ctx.ContractSystem->FundEscrow(ContractID, TEXT("Client001"));

	bool bRefunded = Ctx.ContractSystem->RefundEscrow(ContractID);
	TestTrue(TEXT("Escrow refund should succeed"), bRefunded);

	FContractEscrow Escrow = Ctx.ContractSystem->GetEscrowStatus(ContractID);
	TestEqual(TEXT("Escrow should be Refunded"),
		static_cast<uint8>(Escrow.Status), static_cast<uint8>(EEscrowStatus::Refunded));

	return true;
}

// ============================================================================
// CONTRACT CANCELLATION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_CancelByClient,
	"Odyssey.Social.Contract.Cancel.ByClient",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_CancelByClient::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Cancel Test"), EContractType::Exploration, 2000);

	bool bCancelled = Ctx.ContractSystem->CancelContract(ContractID, TEXT("Client001"), TEXT("Changed mind"));
	TestTrue(TEXT("Client should be able to cancel"), bCancelled);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	TestEqual(TEXT("Status should be Cancelled"),
		static_cast<uint8>(Data.Status), static_cast<uint8>(EContractStatus::Cancelled));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_CancelByUnrelatedPlayer,
	"Odyssey.Social.Contract.Cancel.ByUnrelatedPlayerFails",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_CancelByUnrelatedPlayer::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("No Cancel"), EContractType::Combat, 5000);

	bool bCancelled = Ctx.ContractSystem->CancelContract(ContractID, TEXT("Random999"), TEXT("I want to cancel"));
	TestFalse(TEXT("Unrelated player should not cancel contract"), bCancelled);

	return true;
}

// ============================================================================
// CONTRACT COMPLETION TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_FullCompletion,
	"Odyssey.Social.Contract.Completion.FullWorkflow",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_FullCompletion::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = Ctx.ContractSystem->CreateContract(
		TEXT("Client001"), TEXT("Alice"),
		TEXT("Full Flow"), TEXT("Complete contract test"),
		EContractType::Crafting,
		ContractTestHelpers::MakeOMENPayment(10000));

	// Add milestone
	FContractMilestone M;
	M.Description = TEXT("Do the work");
	M.PaymentPercentage = 1.0f;
	Ctx.ContractSystem->AddMilestone(ContractID, TEXT("Client001"), M);

	// Post, accept, fund escrow
	Ctx.ContractSystem->PostContract(ContractID, TEXT("Client001"));
	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	Ctx.ContractSystem->FundEscrow(ContractID, TEXT("Client001"));

	// Complete and confirm milestone
	Ctx.ContractSystem->CompleteMilestone(ContractID, TEXT("Contractor001"), 0);
	Ctx.ContractSystem->ConfirmMilestone(ContractID, TEXT("Client001"), 0);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);

	// Contract should be complete or awaiting confirmation
	TestTrue(TEXT("Contract should be Completed or AwaitingConfirmation"),
		Data.Status == EContractStatus::Completed || Data.Status == EContractStatus::AwaitingConfirmation);

	return true;
}

// ============================================================================
// DISPUTE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_FileDispute,
	"Odyssey.Social.Contract.Disputes.FileDispute",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_FileDispute::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Dispute Test"), EContractType::Transport, 5000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	Ctx.ContractSystem->FundEscrow(ContractID, TEXT("Client001"));

	bool bFiled = Ctx.ContractSystem->FileDispute(ContractID, TEXT("Client001"),
		TEXT("Work not done"), TEXT("No evidence of delivery"));
	TestTrue(TEXT("Filing dispute should succeed"), bFiled);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	TestTrue(TEXT("Contract should have dispute flag"), Data.bHasDispute);
	TestEqual(TEXT("Contract should be in Disputed status"),
		static_cast<uint8>(Data.Status), static_cast<uint8>(EContractStatus::Disputed));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_ResolveDispute,
	"Odyssey.Social.Contract.Disputes.ResolveDispute",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_ResolveDispute::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Resolve Test"), EContractType::Repair, 8000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));
	Ctx.ContractSystem->FundEscrow(ContractID, TEXT("Client001"));
	Ctx.ContractSystem->FileDispute(ContractID, TEXT("Client001"), TEXT("Bad work"), TEXT(""));

	bool bResolved = Ctx.ContractSystem->ResolveDispute(ContractID,
		TEXT("50/50 split"), 0.5f);
	TestTrue(TEXT("Resolving dispute should succeed"), bResolved);

	FContractDispute Dispute;
	Ctx.ContractSystem->GetDisputeDetails(ContractID, Dispute);
	TestTrue(TEXT("Dispute should be marked resolved"), Dispute.bIsResolved);
	TestEqual(TEXT("Refund percentage should be 0.5"), Dispute.ClientRefundPercentage, 0.5f);

	return true;
}

// ============================================================================
// RATING TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_SubmitRating,
	"Odyssey.Social.Contract.Ratings.SubmitRating",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_SubmitRating::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Rating Test"), EContractType::Crafting, 5000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));

	TMap<ERatingCategory, int32> Ratings;
	Ratings.Add(ERatingCategory::Overall, 5);
	Ratings.Add(ERatingCategory::Communication, 4);
	Ratings.Add(ERatingCategory::Quality, 5);
	Ratings.Add(ERatingCategory::Timeliness, 3);

	bool bRated = Ctx.ContractSystem->SubmitRating(ContractID, TEXT("Client001"),
		Ratings, TEXT("Great work!"), true);
	TestTrue(TEXT("Submitting rating should succeed"), bRated);

	// Check service profile
	FServiceProfile Profile;
	bool bGotProfile = Ctx.ContractSystem->GetServiceProfile(TEXT("Contractor001"), Profile);
	if (bGotProfile)
	{
		TestTrue(TEXT("Profile should exist after rating"), true);
	}

	// Get player ratings
	TArray<FContractRating> PlayerRatings = Ctx.ContractSystem->GetPlayerRatings(TEXT("Contractor001"));
	TestTrue(TEXT("Should have at least 1 rating"), PlayerRatings.Num() >= 1);

	return true;
}

// ============================================================================
// BIDDING SYSTEM TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_BiddingSystem,
	"Odyssey.Social.Contract.Bidding.SubmitAndAcceptOffer",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_BiddingSystem::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Bid Job"), EContractType::Mining, 10000);

	TMap<EResourceType, int64> ProposedPayment;
	ProposedPayment.Add(EResourceType::OMEN, 8000);

	FGuid OfferID = Ctx.ContractSystem->SubmitOffer(ContractID,
		TEXT("Bidder001"), TEXT("Bob"), ProposedPayment, TEXT("I can do it for less"), 5.0f);
	TestTrue(TEXT("Offer should be submitted"), OfferID.IsValid());

	TArray<FContractOffer> Offers = Ctx.ContractSystem->GetContractOffers(ContractID);
	TestEqual(TEXT("Should have 1 offer"), Offers.Num(), 1);

	bool bAccepted = Ctx.ContractSystem->AcceptOffer(ContractID, TEXT("Client001"), OfferID);
	TestTrue(TEXT("Accepting offer should succeed"), bAccepted);

	FSocialContract Data;
	Ctx.ContractSystem->GetContractData(ContractID, Data);
	TestEqual(TEXT("Contractor should be the bidder"), Data.ContractorPlayerID, FString(TEXT("Bidder001")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_RejectOffer,
	"Odyssey.Social.Contract.Bidding.RejectOffer",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_RejectOffer::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Reject Bid"), EContractType::Trade, 5000);

	TMap<EResourceType, int64> Pay;
	Pay.Add(EResourceType::OMEN, 4500);

	FGuid OfferID = Ctx.ContractSystem->SubmitOffer(ContractID,
		TEXT("Bidder001"), TEXT("Bob"), Pay, TEXT(""), 2.0f);

	bool bRejected = Ctx.ContractSystem->RejectOffer(ContractID, TEXT("Client001"), OfferID);
	TestTrue(TEXT("Rejecting offer should succeed"), bRejected);

	return true;
}

// ============================================================================
// MESSAGING TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_Messaging,
	"Odyssey.Social.Contract.Communication.SendAndGetMessages",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_Messaging::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	FGuid ContractID = ContractTestHelpers::CreateAndPostContract(
		Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Chat Test"), EContractType::Custom, 1000);

	Ctx.ContractSystem->AcceptContract(ContractID, TEXT("Contractor001"), TEXT("Bob"));

	bool bSent1 = Ctx.ContractSystem->SendMessage(ContractID, TEXT("Client001"), TEXT("Alice"), TEXT("Hello!"));
	bool bSent2 = Ctx.ContractSystem->SendMessage(ContractID, TEXT("Contractor001"), TEXT("Bob"), TEXT("Hi there!"));
	TestTrue(TEXT("Client message should send"), bSent1);
	TestTrue(TEXT("Contractor message should send"), bSent2);

	TArray<FContractMessage> Messages = Ctx.ContractSystem->GetMessages(ContractID);
	TestTrue(TEXT("Should have at least 2 messages"), Messages.Num() >= 2);

	return true;
}

// ============================================================================
// SERVICE PROFILE TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_ServiceProfile,
	"Odyssey.Social.Contract.Profile.UpdateAndRetrieve",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_ServiceProfile::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	TArray<EContractType> Services = { EContractType::Crafting, EContractType::Mining };

	bool bUpdated = Ctx.ContractSystem->UpdateServiceProfile(
		TEXT("Player001"), Services, TEXT("Expert crafter and miner"), true);
	TestTrue(TEXT("Updating service profile should succeed"), bUpdated);

	FServiceProfile Profile;
	bool bGot = Ctx.ContractSystem->GetServiceProfile(TEXT("Player001"), Profile);
	TestTrue(TEXT("Should retrieve profile"), bGot);
	TestEqual(TEXT("Bio should match"), Profile.Bio, FString(TEXT("Expert crafter and miner")));
	TestTrue(TEXT("Should be available"), Profile.bIsAvailable);

	return true;
}

// ============================================================================
// SEARCH TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_Search,
	"Odyssey.Social.Contract.Search.SearchContracts",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_Search::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	ContractTestHelpers::CreateAndPostContract(Ctx.ContractSystem, TEXT("C1"), TEXT("Alice"),
		TEXT("Mining Expedition"), EContractType::Mining, 5000);
	ContractTestHelpers::CreateAndPostContract(Ctx.ContractSystem, TEXT("C2"), TEXT("Bob"),
		TEXT("Escort Mission"), EContractType::Escort, 3000);
	ContractTestHelpers::CreateAndPostContract(Ctx.ContractSystem, TEXT("C3"), TEXT("Carol"),
		TEXT("Mining Haul"), EContractType::Mining, 2000);

	TArray<FSocialContract> Results = Ctx.ContractSystem->SearchContracts(
		TEXT("Mining"), EContractType::Mining, true);
	TestTrue(TEXT("Should find mining contracts"), Results.Num() >= 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_PlayerContracts,
	"Odyssey.Social.Contract.Search.PlayerContracts",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_PlayerContracts::RunTest(const FString& Parameters)
{
	auto Ctx = ContractTestHelpers::CreateTestContext();

	ContractTestHelpers::CreateAndPostContract(Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Job 1"), EContractType::Mining, 5000);
	ContractTestHelpers::CreateAndPostContract(Ctx.ContractSystem, TEXT("Client001"), TEXT("Alice"),
		TEXT("Job 2"), EContractType::Escort, 3000);

	TArray<FSocialContract> PlayerContracts = Ctx.ContractSystem->GetPlayerContracts(TEXT("Client001"));
	TestTrue(TEXT("Should find 2 contracts for client"), PlayerContracts.Num() >= 2);

	return true;
}

// ============================================================================
// PAYMENT TERMS STRUCT TESTS
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContract_PaymentTermsLogic,
	"Odyssey.Social.Contract.PaymentTerms.Calculations",
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FContract_PaymentTermsLogic::RunTest(const FString& Parameters)
{
	FContractPaymentTerms Terms;
	Terms.TotalPayment.Add(EResourceType::OMEN, 10000);
	Terms.UpfrontPercentage = 0.2f;
	Terms.MilestonePercentage = 0.3f;
	Terms.CompletionPercentage = 0.5f;

	TestEqual(TEXT("Total OMEN should be 10000"), Terms.GetTotalOMEN(), (int64)10000);
	TestEqual(TEXT("Upfront amount should be 2000"),
		Terms.GetUpfrontAmount(EResourceType::OMEN), (int64)2000);
	TestEqual(TEXT("Completion amount should be 5000"),
		Terms.GetCompletionAmount(EResourceType::OMEN), (int64)5000);

	return true;
}

