#include "Inventory/FactoryInventoryComponent.h"

UFactoryInventoryComponent::UFactoryInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFactoryInventoryComponent::IsValidSlot(EFactoryInventorySlot Slot)
{
	return Slot == EFactoryInventorySlot::Input || Slot == EFactoryInventorySlot::Output;
}

const TMap<FName, int32>& UFactoryInventoryComponent::GetStorage(EFactoryInventorySlot Slot) const
{
	return Slot == EFactoryInventorySlot::Input ? InputItems : OutputItems;
}

TMap<FName, int32>& UFactoryInventoryComponent::GetMutableStorage(EFactoryInventorySlot Slot)
{
	return Slot == EFactoryInventorySlot::Input ? InputItems : OutputItems;
}

int32 UFactoryInventoryComponent::GetQuantity(EFactoryInventorySlot Slot, FName ItemId) const
{
	if (!IsValidSlot(Slot) || ItemId.IsNone())
	{
		OperationLog.Fail(this, TEXT("GetQuantity"), TEXT("Invalid slot or empty item ID."), FString::Printf(TEXT("Slot=%d Item=%s"), int32(Slot), *ItemId.ToString()));
		return 0;
	}
	OperationLog.Succeed(TEXT("GetQuantity"));
	return GetStorage(Slot).FindRef(ItemId);
}

int32 UFactoryInventoryComponent::GetCapacity(EFactoryInventorySlot Slot, FName ItemId) const
{
	if (!IsValidSlot(Slot) || ItemId.IsNone())
	{
		OperationLog.Fail(this, TEXT("GetCapacity"), TEXT("Invalid slot or empty item ID."), FString::Printf(TEXT("Slot=%d Item=%s"), int32(Slot), *ItemId.ToString()));
		return 0;
	}
	OperationLog.Succeed(TEXT("GetCapacity"));
	const auto& Capacities = Slot == EFactoryInventorySlot::Input ? InputCapacities : OutputCapacities;
	return FMath::Max(0, Capacities.FindRef(ItemId));
}

int32 UFactoryInventoryComponent::GetFreeSpace(EFactoryInventorySlot Slot, FName ItemId) const
{
	return FMath::Max(0, GetCapacity(Slot, ItemId) - GetQuantity(Slot, ItemId));
}

TArray<FFactoryItemAmount> UFactoryInventoryComponent::GetItems(EFactoryInventorySlot Slot) const
{
	TArray<FFactoryItemAmount> Result;
	if (!IsValidSlot(Slot))
	{
		OperationLog.Fail(this, TEXT("GetItems"), TEXT("Invalid inventory slot."), FString::Printf(TEXT("Slot=%d"), int32(Slot)));
		return Result;
	}
	OperationLog.Succeed(TEXT("GetItems"));
	for (const auto& Pair : GetStorage(Slot))
	{
		FFactoryItemAmount Item;
		Item.ItemId = Pair.Key;
		Item.Quantity = Pair.Value;
		Result.Add(Item);
	}
	Result.Sort([](const FFactoryItemAmount& A, const FFactoryItemAmount& B)
	{
		return A.ItemId.LexicalLess(B.ItemId);
	});
	return Result;
}

bool UFactoryInventoryComponent::AggregateItems(const TArray<FFactoryItemAmount>& Items, TMap<FName, int32>& OutTotals) const
{
	OutTotals.Reset();
	for (const FFactoryItemAmount& Item : Items)
	{
		if (Item.ItemId.IsNone() || Item.Quantity <= 0)
		{
			return OperationLog.Fail(this, TEXT("AggregateItems"), TEXT("Empty item ID or non-positive quantity."), FString::Printf(TEXT("Item=%s Quantity=%d"), *Item.ItemId.ToString(), Item.Quantity));
		}
		const int64 Total = int64(OutTotals.FindRef(Item.ItemId)) + Item.Quantity;
		if (Total > MAX_int32)
		{
			return OperationLog.Fail(this, TEXT("AggregateItems"), TEXT("Combined quantity exceeds int32 range."), FString::Printf(TEXT("Item=%s Total=%lld"), *Item.ItemId.ToString(), Total));
		}
		OutTotals.Add(Item.ItemId, int32(Total));
	}
	OperationLog.Succeed(TEXT("AggregateItems"));
	return true;
}

bool UFactoryInventoryComponent::HasItems(EFactoryInventorySlot Slot, const TArray<FFactoryItemAmount>& Items) const
{
	TMap<FName, int32> Totals;
	if (!IsValidSlot(Slot))
	{
		return OperationLog.Fail(this, TEXT("HasItems"), TEXT("Invalid inventory slot."), FString::Printf(TEXT("Slot=%d"), int32(Slot)));
	}
	if (!AggregateItems(Items, Totals)) return false;
	for (const auto& Pair : Totals)
	{
		if (GetQuantity(Slot, Pair.Key) < Pair.Value)
		{
			return OperationLog.Fail(this, TEXT("HasItems"), TEXT("Insufficient inventory for item ") + Pair.Key.ToString(), FString::Printf(TEXT("Slot=%d Required=%d Available=%d"), int32(Slot), Pair.Value, GetQuantity(Slot, Pair.Key)), EFactoryLogLevel::Verbose);
		}
	}
	OperationLog.Succeed(TEXT("HasItems"));
	return true;
}

bool UFactoryInventoryComponent::SetCapacity(EFactoryInventorySlot Slot, FName ItemId, int32 Capacity)
{
	if (!IsValidSlot(Slot) || ItemId.IsNone() || Capacity < 0)
	{
		return OperationLog.Fail(this, TEXT("SetCapacity"), TEXT("Invalid slot, empty item ID or negative capacity."), FString::Printf(TEXT("Slot=%d Item=%s Capacity=%d"), int32(Slot), *ItemId.ToString(), Capacity));
	}
	OperationLog.Succeed(TEXT("SetCapacity"));
	auto& Capacities = Slot == EFactoryInventorySlot::Input ? InputCapacities : OutputCapacities;
	if (Capacities.FindRef(ItemId) != Capacity)
	{
		Capacities.Add(ItemId, Capacity);
		UE_LOG(LogFactoryGame, Log, TEXT("Inventory capacity changed [Context=%s Slot=%d Item=%s Capacity=%d]"), *GetPathName(), int32(Slot), *ItemId.ToString(), Capacity);
		OnInventoryChanged.Broadcast();
	}
	return true;
}

int32 UFactoryInventoryComponent::AddItems(EFactoryInventorySlot Slot, FName ItemId, int32 Quantity)
{
	if (!IsValidSlot(Slot) || ItemId.IsNone() || Quantity <= 0)
	{
		OperationLog.Fail(this, TEXT("AddItems"), TEXT("Invalid slot, empty item ID or non-positive quantity."), FString::Printf(TEXT("Slot=%d Item=%s Quantity=%d"), int32(Slot), *ItemId.ToString(), Quantity));
		return 0;
	}
	const int32 Added = FMath::Min(Quantity, GetFreeSpace(Slot, ItemId));
	if (Added < Quantity)
	{
		OperationLog.Fail(this, TEXT("AddItems"), TEXT("Insufficient storage space."), FString::Printf(TEXT("Slot=%d Item=%s Requested=%d Actual=%d"), int32(Slot), *ItemId.ToString(), Quantity, Added), EFactoryLogLevel::Verbose);
	}
	else
	{
		OperationLog.Succeed(TEXT("AddItems"));
	}
	if (Added > 0)
	{
		GetMutableStorage(Slot).FindOrAdd(ItemId) += Added;
		OnInventoryChanged.Broadcast();
	}
	return Added;
}

int32 UFactoryInventoryComponent::RemoveItems(EFactoryInventorySlot Slot, FName ItemId, int32 Quantity)
{
	if (!IsValidSlot(Slot) || ItemId.IsNone() || Quantity <= 0)
	{
		OperationLog.Fail(this, TEXT("RemoveItems"), TEXT("Invalid slot, empty item ID or non-positive quantity."), FString::Printf(TEXT("Slot=%d Item=%s Quantity=%d"), int32(Slot), *ItemId.ToString(), Quantity));
		return 0;
	}
	const int32 Removed = FMath::Min(Quantity, GetQuantity(Slot, ItemId));
	if (Removed < Quantity)
	{
		OperationLog.Fail(this, TEXT("RemoveItems"), TEXT("Insufficient stock."), FString::Printf(TEXT("Slot=%d Item=%s Requested=%d Actual=%d"), int32(Slot), *ItemId.ToString(), Quantity, Removed), EFactoryLogLevel::Verbose);
	}
	else
	{
		OperationLog.Succeed(TEXT("RemoveItems"));
	}
	if (Removed > 0)
	{
		auto& Storage = GetMutableStorage(Slot);
		int32& Remaining = Storage.FindChecked(ItemId);
		Remaining -= Removed;
		if (Remaining == 0) Storage.Remove(ItemId);
		OnInventoryChanged.Broadcast();
	}
	return Removed;
}

bool UFactoryInventoryComponent::TryConsumeItems(EFactoryInventorySlot Slot, const TArray<FFactoryItemAmount>& Items)
{
	TMap<FName, int32> Totals;
	if (!IsValidSlot(Slot))
	{
		return OperationLog.Fail(this, TEXT("TryConsumeItems"), TEXT("Invalid inventory slot."), FString::Printf(TEXT("Slot=%d"), int32(Slot)));
	}
	if (!AggregateItems(Items, Totals)) return false;
	for (const auto& Pair : Totals)
	{
		if (GetQuantity(Slot, Pair.Key) < Pair.Value)
		{
			return OperationLog.Fail(this, TEXT("TryConsumeItems"), TEXT("Insufficient inventory for item ") + Pair.Key.ToString(), FString::Printf(TEXT("Slot=%d Required=%d Available=%d"), int32(Slot), Pair.Value, GetQuantity(Slot, Pair.Key)), EFactoryLogLevel::Verbose);
		}
	}
	auto& Storage = GetMutableStorage(Slot);
	for (const auto& Pair : Totals)
	{
		int32& Remaining = Storage.FindChecked(Pair.Key);
		Remaining -= Pair.Value;
		if (Remaining == 0) Storage.Remove(Pair.Key);
	}
	// 모든 재료의 차감이 끝난 뒤 한 번만 통지한다.
	if (!Totals.IsEmpty()) OnInventoryChanged.Broadcast();
	OperationLog.Succeed(TEXT("TryConsumeItems"));
	return true;
}
