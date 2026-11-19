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
	return IsValidSlot(Slot) && !ItemId.IsNone() ? GetStorage(Slot).FindRef(ItemId) : 0;
}

int32 UFactoryInventoryComponent::GetCapacity(EFactoryInventorySlot Slot, FName ItemId) const
{
	if (!IsValidSlot(Slot) || ItemId.IsNone()) return 0;
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
	if (!IsValidSlot(Slot)) return Result;
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

bool UFactoryInventoryComponent::AggregateItems(const TArray<FFactoryItemAmount>& Items, TMap<FName, int32>& OutTotals)
{
	OutTotals.Reset();
	for (const FFactoryItemAmount& Item : Items)
	{
		if (Item.ItemId.IsNone() || Item.Quantity <= 0) return false;
		const int64 Total = int64(OutTotals.FindRef(Item.ItemId)) + Item.Quantity;
		if (Total > MAX_int32) return false;
		OutTotals.Add(Item.ItemId, int32(Total));
	}
	return true;
}

bool UFactoryInventoryComponent::HasItems(EFactoryInventorySlot Slot, const TArray<FFactoryItemAmount>& Items) const
{
	TMap<FName, int32> Totals;
	if (!IsValidSlot(Slot) || !AggregateItems(Items, Totals)) return false;
	for (const auto& Pair : Totals)
	{
		if (GetQuantity(Slot, Pair.Key) < Pair.Value) return false;
	}
	return true;
}

bool UFactoryInventoryComponent::SetCapacity(EFactoryInventorySlot Slot, FName ItemId, int32 Capacity)
{
	if (!IsValidSlot(Slot) || ItemId.IsNone() || Capacity < 0) return false;
	auto& Capacities = Slot == EFactoryInventorySlot::Input ? InputCapacities : OutputCapacities;
	if (Capacities.FindRef(ItemId) != Capacity)
	{
		Capacities.Add(ItemId, Capacity);
		OnInventoryChanged.Broadcast();
	}
	return true;
}

int32 UFactoryInventoryComponent::AddItems(EFactoryInventorySlot Slot, FName ItemId, int32 Quantity)
{
	if (!IsValidSlot(Slot) || ItemId.IsNone() || Quantity <= 0) return 0;
	const int32 Added = FMath::Min(Quantity, GetFreeSpace(Slot, ItemId));
	if (Added > 0)
	{
		GetMutableStorage(Slot).FindOrAdd(ItemId) += Added;
		OnInventoryChanged.Broadcast();
	}
	return Added;
}

int32 UFactoryInventoryComponent::RemoveItems(EFactoryInventorySlot Slot, FName ItemId, int32 Quantity)
{
	if (!IsValidSlot(Slot) || ItemId.IsNone() || Quantity <= 0) return 0;
	const int32 Removed = FMath::Min(Quantity, GetQuantity(Slot, ItemId));
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
	if (!IsValidSlot(Slot) || !AggregateItems(Items, Totals)) return false;
	for (const auto& Pair : Totals)
	{
		if (GetQuantity(Slot, Pair.Key) < Pair.Value) return false;
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
	return true;
}
