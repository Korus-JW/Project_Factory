#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/FactoryItemAmount.h"
#include "FactoryInventoryComponent.generated.h"

/** 재료 입력과 완제품 출력을 서로 독립된 저장소로 구분한다. */
UENUM(BlueprintType)
enum class EFactoryInventorySlot : uint8
{
	Input,
	Output
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFactoryInventoryChanged);

/** 품목별 한도와 재고를 관리한다. 미설정 품목의 저장 한도는 0이다. */
UCLASS(ClassGroup = (Factory), meta = (BlueprintSpawnableComponent))
class PROJECT_FACTORY_API UFactoryInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFactoryInventoryComponent();

	/** 수량 또는 저장 한도가 변경되면 호출한다. 변경 후 조회 함수로 상태를 읽는다. */
	UPROPERTY(BlueprintAssignable, Category = "Factory|Inventory")
	FOnFactoryInventoryChanged OnInventoryChanged;

	UFUNCTION(BlueprintPure, Category = "Factory|Inventory")
	int32 GetQuantity(EFactoryInventorySlot Slot, FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Factory|Inventory")
	int32 GetCapacity(EFactoryInventorySlot Slot, FName ItemId) const;

	UFUNCTION(BlueprintPure, Category = "Factory|Inventory")
	int32 GetFreeSpace(EFactoryInventorySlot Slot, FName ItemId) const;

	/** 실제 보관 중인 양만 반환한다. 반환된 배열 수정은 원본 재고에 영향을 주지 않는다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Inventory")
	TArray<FFactoryItemAmount> GetItems(EFactoryInventorySlot Slot) const;

	/** 같은 품목이 여러 번 나오면 합산한다. 빈 목록은 충족하며 잘못된 항목은 실패한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Inventory")
	bool HasItems(EFactoryInventorySlot Slot, const TArray<FFactoryItemAmount>& Items) const;

	/** 한도를 줄여도 기존 물품은 보존한다. 음수 한도와 빈 ID는 거부한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Inventory")
	bool SetCapacity(EFactoryInventorySlot Slot, FName ItemId, int32 Capacity);

	/** 공간 범위에서 추가하고 실제 추가량을 반환한다. 잘못된 요청은 0을 반환한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Inventory")
	int32 AddItems(EFactoryInventorySlot Slot, FName ItemId, int32 Quantity);

	/** 재고 범위에서 제거하고 실제 제거량을 반환한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Inventory")
	int32 RemoveItems(EFactoryInventorySlot Slot, FName ItemId, int32 Quantity);

	/** 모든 재료가 있을 때만 한 번에 소비한다. 실패 시 어떤 재료도 차감하지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Inventory")
	bool TryConsumeItems(EFactoryInventorySlot Slot, const TArray<FFactoryItemAmount>& Items);

private:
	/** 자식 건물 블루프린트의 컴포넌트 기본값에서 품목별로 설정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Factory|Inventory", meta = (ClampMin = "0"))
	TMap<FName, int32> InputCapacities;

	UPROPERTY(EditDefaultsOnly, Category = "Factory|Inventory", meta = (ClampMin = "0"))
	TMap<FName, int32> OutputCapacities;

	/** 실제 저장 파일 직렬화는 저장 시스템에서 수행한다. */
	UPROPERTY(SaveGame)
	TMap<FName, int32> InputItems;

	UPROPERTY(SaveGame)
	TMap<FName, int32> OutputItems;

	static bool IsValidSlot(EFactoryInventorySlot Slot);
	static bool AggregateItems(const TArray<FFactoryItemAmount>& Items, TMap<FName, int32>& OutTotals);
	const TMap<FName, int32>& GetStorage(EFactoryInventorySlot Slot) const;
	TMap<FName, int32>& GetMutableStorage(EFactoryInventorySlot Slot);
};
