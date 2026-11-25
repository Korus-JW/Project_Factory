#include "Production/FactoryProductionComponent.h"
#include "Inventory/FactoryInventoryComponent.h"
#include "Buildings/FactoryBuildingBase.h"


namespace
{
	// 중복 품목을 합산하고 잘못된 수량과 정수 범위 초과를 거부한다.
	bool NormalizeAmounts(const TArray<FFactoryItemAmount>& Source, TArray<FFactoryItemAmount>& Result, FString* OutFailureReason = nullptr)
	{
		TMap<FName, int32> Totals;
		for (const auto& Item : Source)
		{
			const int64 Total = int64(Totals.FindRef(Item.ItemId)) + Item.Quantity;
			if (Item.ItemId.IsNone() || Item.Quantity <= 0 || Total > MAX_int32)
			{
				if (OutFailureReason)
				{
					const TCHAR* Reason = Item.ItemId.IsNone() ? TEXT("ItemId is None")
						: Item.Quantity <= 0 ? TEXT("Quantity must be positive") : TEXT("Combined item quantity exceeds int32 range");
					*OutFailureReason = FString::Printf(TEXT("%s (ItemId=%s, Quantity=%d, Total=%lld)"),
						Reason, *Item.ItemId.ToString(), Item.Quantity, Total);
				}
				return false;
			}
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
	Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UFactoryInventoryComponent>() : nullptr;
	if (bHasRecipe || (!DefaultRecipe.DataTable && DefaultRecipe.RowName.IsNone())) return;
	if (!AvailableRecipes.IsEmpty())
	{
		SelectRecipe(DefaultRecipe);
		return;
	}
	if (!DefaultRecipe.DataTable || DefaultRecipe.RowName.IsNone()
		|| DefaultRecipe.DataTable->GetRowStruct() != FFactoryRecipeDefinition::StaticStruct())
	{
		OperationLog.Fail(this, TEXT("DefaultRecipe"), TEXT("Missing table, row name or incompatible row structure."));
		return;
	}
	const auto* Recipe = DefaultRecipe.DataTable->FindRow<FFactoryRecipeDefinition>(DefaultRecipe.RowName, TEXT("FactoryProduction"), false);
	if (!Recipe)
	{
		OperationLog.Fail(this, TEXT("DefaultRecipe"), TEXT("Recipe row not found."), DefaultRecipe.RowName.ToString());
		return;
	}
	if (SetRecipe(*Recipe)) SelectedRecipe = DefaultRecipe;
}

bool UFactoryProductionComponent::SelectRecipe(FDataTableRowHandle Recipe)
{
	const auto Fail = [this, &Recipe](const FString& Reason)
	{
		return OperationLog.Fail(this, TEXT("SelectRecipe"), Reason,
			FString::Printf(TEXT("Table=%s Row=%s"), *GetPathNameSafe(Recipe.DataTable), *Recipe.RowName.ToString()));
	};
	if (bAdvancing) return Fail(TEXT("Production update is in progress."));
	if (!AvailableRecipes.ContainsByPredicate([&](const FDataTableRowHandle& Entry)
		{ return Entry.DataTable == Recipe.DataTable && Entry.RowName == Recipe.RowName; }))
	{
		return Fail(TEXT("Recipe is not in AvailableRecipes."));
	}
	if (!Recipe.DataTable || Recipe.RowName.IsNone()) return Fail(TEXT("Missing recipe table or row name."));
	if (Recipe.DataTable->GetRowStruct() != FFactoryRecipeDefinition::StaticStruct()) return Fail(TEXT("Incompatible recipe row structure."));
	const auto* Definition = Recipe.DataTable->FindRow<FFactoryRecipeDefinition>(Recipe.RowName, TEXT("SelectRecipe"), false);
	if (!Definition) return Fail(TEXT("Recipe row not found."));
	if (!FMath::IsFinite(Definition->ProductionTimeSeconds) || Definition->ProductionTimeSeconds < 0.001f)
	{
		return Fail(FString::Printf(TEXT("Production time must be finite and at least 0.001 seconds. Value=%g"), Definition->ProductionTimeSeconds));
	}
	if (Definition->Outputs.IsEmpty()) return Fail(TEXT("Recipe has no outputs."));
	FFactoryRecipeDefinition Normalized = *Definition;
	FString FailureReason;
	if (!NormalizeAmounts(Definition->Inputs, Normalized.Inputs, &FailureReason)) return Fail(TEXT("Invalid inputs: ") + FailureReason);
	if (!NormalizeAmounts(Definition->Outputs, Normalized.Outputs, &FailureReason)) return Fail(TEXT("Invalid outputs: ") + FailureReason);
	OperationLog.Succeed(TEXT("SelectRecipe"));
	// 현재 제작법을 다시 선택하면 이전 변경 예약을 취소한다.
	if (SelectedRecipe.DataTable == Recipe.DataTable && SelectedRecipe.RowName == Recipe.RowName)
	{
		bHasPendingRecipe = false;
		PendingRecipe = FDataTableRowHandle();
		return true;
	}
	UE_LOG(LogFactoryGame, Log, TEXT("Recipe change requested [Context=%s Row=%s]"), *GetPathName(), *Recipe.RowName.ToString());
	PendingRecipe = Recipe;
	PendingRecipeDefinition = MoveTemp(Normalized);
	bHasPendingRecipe = true;
	if (!bWorking && PendingOutputs.IsEmpty()) ApplyPendingRecipe();
	return true;
}

void UFactoryProductionComponent::ApplyPendingRecipe()
{
	if (!bHasPendingRecipe) return;
	UE_LOG(LogFactoryGame, Log, TEXT("Applying recipe [Context=%s Row=%s]"), *GetPathName(), *PendingRecipe.RowName.ToString());
	ActiveRecipe = MoveTemp(PendingRecipeDefinition);
	SelectedRecipe = PendingRecipe;
	PendingRecipe = FDataTableRowHandle();
	bHasPendingRecipe = false;
	bHasRecipe = true;
	State = bProductionEnabled ? EFactoryProductionState::WaitingForMaterials : EFactoryProductionState::Paused;
}

bool UFactoryProductionComponent::SetRecipe(const FFactoryRecipeDefinition& Recipe)
{
	// 첫 번째 실패 원인과 호출 대상 정보를 함께 남긴다.
	const auto Fail = [this, &Recipe](const FString& Reason)
	{
		return OperationLog.Fail(this, TEXT("SetRecipe"), Reason,
			FString::Printf(TEXT("Recipe=%s"), *Recipe.DisplayName.ToString()));
	};
	if (!AvailableRecipes.IsEmpty()) return Fail(TEXT("AvailableRecipes is configured. Use SelectRecipe to choose a listed recipe."));
	if (bWorking) return Fail(TEXT("Production or output delivery is in progress. Wait for completion or queue a change with SelectRecipe."));
	if (bAdvancing) return Fail(TEXT("Cannot change a recipe during production update."));
	if (!PendingOutputs.IsEmpty()) return Fail(TEXT("Undelivered outputs remain."));
	if (!FMath::IsFinite(Recipe.ProductionTimeSeconds) || Recipe.ProductionTimeSeconds < 0.001f)
		return Fail(FString::Printf(TEXT("ProductionTimeSeconds must be finite and at least 0.001. Value=%g"), Recipe.ProductionTimeSeconds));
	if (Recipe.Outputs.IsEmpty()) return Fail(TEXT("Outputs is empty. At least one output is required."));
	FFactoryRecipeDefinition Normalized = Recipe;
	FString FailureReason;
	if (!NormalizeAmounts(Recipe.Inputs, Normalized.Inputs, &FailureReason)) return Fail(TEXT("Invalid inputs: ") + FailureReason);
	if (!NormalizeAmounts(Recipe.Outputs, Normalized.Outputs, &FailureReason)) return Fail(TEXT("Invalid outputs: ") + FailureReason);
	OperationLog.Succeed(TEXT("SetRecipe"));
	UE_LOG(LogFactoryGame, Log, TEXT("Recipe configured [Context=%s Recipe=%s]"), *GetPathName(), *Recipe.DisplayName.ToString());
	ActiveRecipe = MoveTemp(Normalized);
	SelectedRecipe = FDataTableRowHandle();
	bHasRecipe = true;
	State = bProductionEnabled ? EFactoryProductionState::WaitingForMaterials : EFactoryProductionState::Paused;
	return true;
}

void UFactoryProductionComponent::SetProductionEnabled(bool bEnabled)
{
	if (bProductionEnabled != bEnabled)
	{
		UE_LOG(LogFactoryGame, Log, TEXT("Production enabled changed [Context=%s Enabled=%d]"), *GetPathName(), bEnabled);
	}
	bProductionEnabled = bEnabled;
	if (!bEnabled) SetState(EFactoryProductionState::Paused, TEXT("Production disabled or placement preview active."));
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
		const int32 FreeSpace = Inventory->GetFreeSpace(EFactoryInventorySlot::Output, Item.ItemId);
		if (FreeSpace < Item.Quantity)
		{
			return OperationLog.Fail(this, TEXT("HasOutputSpace"), TEXT("Insufficient output space for item ") + Item.ItemId.ToString(),
				FString::Printf(TEXT("Required=%d Free=%d"), Item.Quantity, FreeSpace));
		}
	}
	OperationLog.Succeed(TEXT("HasOutputSpace"));
	return true;
}

void UFactoryProductionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceProduction(DeltaTime);
}

void UFactoryProductionComponent::AdvanceProduction(float DeltaTime)
{
	if (bAdvancing)
	{
		OperationLog.Fail(this, TEXT("AdvanceProductionReentry"), TEXT("Reentrant production update rejected."));
		return;
	}
	OperationLog.Succeed(TEXT("AdvanceProductionReentry"));
	if (!FMath::IsFinite(DeltaTime) || DeltaTime < 0.0f)
	{
		OperationLog.Fail(this, TEXT("AdvanceProductionTime"), TEXT("DeltaTime must be finite and non-negative."), FString::Printf(TEXT("DeltaTime=%g"), DeltaTime));
		return;
	}
	OperationLog.Succeed(TEXT("AdvanceProductionTime"));
	TGuardValue<bool> Guard(bAdvancing, true);
	const auto* Building = Cast<AFactoryBuildingBase>(GetOwner());
	if (!bProductionEnabled || (Building && Building->IsPlacementPreview())) { SetState(EFactoryProductionState::Paused, TEXT("Production disabled or placement preview active.")); return; }
	if (!bHasRecipe) { SetState(EFactoryProductionState::NoRecipe, TEXT("No recipe configured.")); return; }
	if (!IsValid(Inventory)) { SetState(EFactoryProductionState::MissingInventory, TEXT("Inventory component is missing or invalid.")); return; }
	// 긴 프레임에서도 남은 시간을 다음 작업에 적용하되 한 프레임의 처리량은 제한한다.
	for (int32 Cycle = 0; Cycle < 1024 && bProductionEnabled; ++Cycle)
	{
		if (!bWorking)
		{
			if (!HasOutputSpace()) { SetState(EFactoryProductionState::WaitingForOutputSpace, TEXT("Output storage has insufficient free space.")); return; }
			if (!Inventory->TryConsumeItems(EFactoryInventorySlot::Input, ActiveRecipe.Inputs))
			{ SetState(EFactoryProductionState::WaitingForMaterials, TEXT("Required input materials are unavailable.")); return; }
			bWorking = true;
			ElapsedSeconds = 0.0f;
			PendingOutputs = ActiveRecipe.Outputs;
		}
		if (!bProductionEnabled) { SetState(EFactoryProductionState::Paused, TEXT("Production disabled or placement preview active.")); return; }
		// 입고만 대기하는 완성 작업은 매 프레임 생산 상태로 되돌리지 않는다.
		if (ElapsedSeconds < ActiveRecipe.ProductionTimeSeconds)
		{
			SetState(EFactoryProductionState::Producing, TEXT("Production is running."));
		}
		const float Step = FMath::Min(DeltaTime, ActiveRecipe.ProductionTimeSeconds - ElapsedSeconds);
		ElapsedSeconds += Step;
		DeltaTime -= Step;
		if (ElapsedSeconds < ActiveRecipe.ProductionTimeSeconds) return;
		for (auto& Item : PendingOutputs)
		{
			if (Item.Quantity > 0) Item.Quantity -= Inventory->AddItems(EFactoryInventorySlot::Output, Item.ItemId, Item.Quantity);
		}
		PendingOutputs.RemoveAll([](const FFactoryItemAmount& Item) { return Item.Quantity == 0; });
		if (!PendingOutputs.IsEmpty()) { SetState(EFactoryProductionState::WaitingForOutputSpace, TEXT("Output storage has insufficient free space.")); return; }
		bWorking = false;
		ElapsedSeconds = 0.0f;
		ApplyPendingRecipe();
		if (DeltaTime <= 0.0f) return;
	}
	if (bProductionEnabled && DeltaTime > 0.0f)
	{
		OperationLog.Fail(this, TEXT("ProductionCycleLimit"), TEXT("Per-update production cycle limit reached; remaining time was discarded."),
			FString::Printf(TEXT("RemainingSeconds=%g Limit=1024"), DeltaTime));
	}
}

void UFactoryProductionComponent::SetState(EFactoryProductionState NewState, const FString& Reason)
{
	if (State == NewState) return;
	State = NewState;
	if (NewState == EFactoryProductionState::MissingInventory)
	{
		UE_LOG(LogFactoryGame, Error, TEXT("Production stopped [Context=%s]: %s"), *GetPathName(), *Reason);
	}
	else
	{
		UE_LOG(LogFactoryGame, Log, TEXT("Production state changed [Context=%s State=%d]: %s"), *GetPathName(), int32(NewState), *Reason);
	}
}
