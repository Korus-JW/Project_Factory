#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Items/FactoryItemAmount.h"
#include "FactoryRecipeDefinition.generated.h"

/** 제작법의 공통 정보다. 데이터 테이블의 행 이름을 제작법 ID로 사용한다. */
USTRUCT(BlueprintType)
struct PROJECT_FACTORY_API FFactoryRecipeDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Recipe")
	FText DisplayName;

	/** 채취 제작법은 입력이 없다. 배열로 여러 종류의 조합 재료를 표현한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Recipe", meta = (TitleProperty = "ItemId"))
	TArray<FFactoryItemAmount> Inputs;

	/** 하나 이상의 산출물을 설정한다. 정제 공정의 복수 산출물도 표현할 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Recipe", meta = (TitleProperty = "ItemId"))
	TArray<FFactoryItemAmount> Outputs;

	/** 한 번 생산하는 데 필요한 게임 시간의 초 단위 값이다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Recipe", meta = (ClampMin = "0.001", UIMin = "0.001", Units = "s"))
	float ProductionTimeSeconds = 1.0f;
};
