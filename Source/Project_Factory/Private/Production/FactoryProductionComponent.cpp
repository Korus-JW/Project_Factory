#include "Production/FactoryProductionComponent.h"
#include "Inventory/FactoryInventoryComponent.h"
#include "Buildings/FactoryBuildingBase.h"

namespace
{
	// 중복 품목을 합산하고 잘못된 수량과 정수 범위 초과를 거부한다.
	bool NormalizeAmounts(const TArray<FFactoryItemAmount>& Source, TArray<FFactoryItemAmount>& Result)
	{
		TMap<FName, int32> Totals;
		for (const auto& Item : Source)
		{
			const int64 Total = int64(Totals.FindRef(Item.ItemId)) + Item.Quantity;
			if (Item.ItemId.IsNone() || Item.Quantity <= 0 || Total > MAX_int32) return false;
			Totals.Add(Item.ItemId, int32(Total));
		}
		Result.Reset();
		for (const auto& Pair : Totals)
		{
			FFactoryItemAmount Item;
			Item.ItemId = Pair.Key;
			Item.Quantity = Pair.Value;
			Result.Add(Item);
		}
		return true;
	}
}

UFactoryProductionComponent::UFactoryProductionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFactoryProductionComponent::BeginPlay()
{
	Super::BeginPlay();
	Inventory = GetOwner()->FindComponentByClass<UFactoryInventoryComponent>();
	if (!bHasRecipe && DefaultRecipe.DataTable && !DefaultRecipe.RowName.IsNone())
	{
		if (!AvailableRecipes.IsEmpty()) SelectRecipe(DefaultRecipe);
		else if (const auto* Recipe = DefaultRecipe.GetRow<FFactoryRecipeDefinition>(TEXT("FactoryProduction")))
		{
			if (SetRecipe(*Recipe)) SelectedRecipe = DefaultRecipe;
		}
	}
}

bool UFactoryProductionComponent::SelectRecipe(FDataTableRowHandle Recipe)
{
	if (bAdvancing || !AvailableRecipes.ContainsByPredicate([&](const FDataTableRowHandle& Entry)
		{ return Entry.DataTable == Recipe.DataTable && Entry.RowName == Recipe.RowName; })) return false;
	if (!Recipe.DataTable || Recipe.RowName.IsNone()
		|| Recipe.DataTable->GetRowStruct() != FFactoryRecipeDefinition::StaticStruct()) return false;
	const auto* Definition = Recipe.DataTable->FindRow<FFactoryRecipeDefinition>(Recipe.RowName, TEXT("SelectRecipe"), false);
	if (!Definition || !FMath::IsFinite(Definition->ProductionTimeSeconds)
		|| Definition->ProductionTimeSeconds < 0.001f || Definition->Outputs.IsEmpty()) return false;
	FFactoryRecipeDefinition Normalized = *Definition;
	if (!NormalizeAmounts(Definition->Inputs, Normalized.Inputs)
		|| !NormalizeAmounts(Definition->Outputs, Normalized.Outputs)) return false;
	// 현재 제작법을 다시 선택하면 이전 변경 예약을 취소한다.
	if (SelectedRecipe.DataTable == Recipe.DataTable && SelectedRecipe.RowName == Recipe.RowName)
	{
		bHasPendingRecipe = false;
		PendingRecipe = FDataTableRowHandle();
		return true;
	}
	PendingRecipe = Recipe;
	PendingRecipeDefinition = MoveTemp(Normalized);
	bHasPendingRecipe = true;
	if (!bWorking && PendingOutputs.IsEmpty()) ApplyPendingRecipe();
	return true;
}

void UFactoryProductionComponent::ApplyPendingRecipe()
{
	if (!bHasPendingRecipe) return;
	ActiveRecipe = MoveTemp(PendingRecipeDefinition);
	SelectedRecipe = PendingRecipe;
	PendingRecipe = FDataTableRowHandle();
	bHasPendingRecipe = false;
	bHasRecipe = true;
	State = bProductionEnabled ? EFactoryProductionState::WaitingForMaterials : EFactoryProductionState::Paused;
}

bool UFactoryProductionComponent::SetRecipe(const FFactoryRecipeDefinition& Recipe)
{
	if (!AvailableRecipes.IsEmpty() || bWorking || bAdvancing || !PendingOutputs.IsEmpty() || !FMath::IsFinite(Recipe.ProductionTimeSeconds)
		|| Recipe.ProductionTimeSeconds < 0.001f || Recipe.Outputs.IsEmpty()) return false;
	FFactoryRecipeDefinition Normalized = Recipe;
	if (!NormalizeAmounts(Recipe.Inputs, Normalized.Inputs) || !NormalizeAmounts(Recipe.Outputs, Normalized.Outputs)) return false;
	ActiveRecipe = MoveTemp(Normalized);
	SelectedRecipe = FDataTableRowHandle();
	bHasRecipe = true;
	State = bProductionEnabled ? EFactoryProductionState::WaitingForMaterials : EFactoryProductionState::Paused;
	return true;
}

void UFactoryProductionComponent::SetProductionEnabled(bool bEnabled)
{
	bProductionEnabled = bEnabled;
	if (!bEnabled) State = EFactoryProductionState::Paused;
}

float UFactoryProductionComponent::GetProgress() const
{
	return bWorking ? FMath::Clamp(ElapsedSeconds / ActiveRecipe.ProductionTimeSeconds, 0.0f, 1.0f) : 0.0f;
}

TArray<FFactoryItemAmount> UFactoryProductionComponent::GetPendingOutputs() const
{
	return bWorking && ElapsedSeconds >= ActiveRecipe.ProductionTimeSeconds ? PendingOutputs : TArray<FFactoryItemAmount>();
}

bool UFactoryProductionComponent::HasOutputSpace() const
{
	for (const auto& Item : ActiveRecipe.Outputs)
	{
		if (Inventory->GetFreeSpace(EFactoryInventorySlot::Output, Item.ItemId) < Item.Quantity) return false;
	}
	return true;
}

void UFactoryProductionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceProduction(DeltaTime);
}

void UFactoryProductionComponent::AdvanceProduction(float DeltaTime)
{
	if (bAdvancing || !FMath::IsFinite(DeltaTime) || DeltaTime < 0.0f) return;
	TGuardValue<bool> Guard(bAdvancing, true);
	const auto* Building = Cast<AFactoryBuildingBase>(GetOwner());
	if (!bProductionEnabled || (Building && Building->IsPlacementPreview())) { State = EFactoryProductionState::Paused; return; }
	if (!bHasRecipe) { State = EFactoryProductionState::NoRecipe; return; }
	if (!IsValid(Inventory)) { State = EFactoryProductionState::MissingInventory; return; }
	// 긴 프레임에서도 남은 시간을 다음 작업에 적용하되 한 프레임의 처리량은 제한한다.
	for (int32 Cycle = 0; Cycle < 1024 && bProductionEnabled; ++Cycle)
	{
		if (!bWorking)
		{
			if (!HasOutputSpace()) { State = EFactoryProductionState::WaitingForOutputSpace; return; }
			if (!Inventory->TryConsumeItems(EFactoryInventorySlot::Input, ActiveRecipe.Inputs))
			{ State = EFactoryProductionState::WaitingForMaterials; return; }
			bWorking = true;
			ElapsedSeconds = 0.0f;
			PendingOutputs = ActiveRecipe.Outputs;
		}
		if (!bProductionEnabled) { State = EFactoryProductionState::Paused; return; }
		State = EFactoryProductionState::Producing;
		const float Step = FMath::Min(DeltaTime, ActiveRecipe.ProductionTimeSeconds - ElapsedSeconds);
		ElapsedSeconds += Step;
		DeltaTime -= Step;
		if (ElapsedSeconds < ActiveRecipe.ProductionTimeSeconds) return;
		for (auto& Item : PendingOutputs)
		{
			Item.Quantity -= Inventory->AddItems(EFactoryInventorySlot::Output, Item.ItemId, Item.Quantity);
		}
		PendingOutputs.RemoveAll([](const FFactoryItemAmount& Item) { return Item.Quantity == 0; });
		if (!PendingOutputs.IsEmpty()) { State = EFactoryProductionState::WaitingForOutputSpace; return; }
		bWorking = false;
		ElapsedSeconds = 0.0f;
		ApplyPendingRecipe();
		if (DeltaTime <= 0.0f) return;
	}
}
