#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FactoryItemDefinition.generated.h"

class UTexture2D;

/** 아이템 목록을 분류하고 필터링할 때 사용하는 구분이다. */
UENUM(BlueprintType)
enum class EFactoryItemCategory : uint8
{
	RawMaterial UMETA(DisplayName = "원재료"),
	Intermediate UMETA(DisplayName = "중간재"),
	FinishedProduct UMETA(DisplayName = "완제품")
};

/** 아이템의 공통 정보다. 데이터 테이블의 행 이름을 아이템 ID로 사용한다. */
USTRUCT(BlueprintType)
struct PROJECT_FACTORY_API FFactoryItemDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item")
	EFactoryItemCategory Category = EFactoryItemCategory::RawMaterial;

	/** 기본 판매 단가다. 실제 가격 균형은 프로토타입에서 조정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item", meta = (ClampMin = "0", UIMin = "0"))
	int32 BaseSellPrice = 0;

	/** 물품 한 단위가 차지하는 운송 적재량이다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 UnitCargoSize = 1;
};
