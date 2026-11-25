#pragma once

#include "CoreMinimal.h"
#include "TerrainChunkData.generated.h"

UENUM(BlueprintType)
enum class ETerrainType : uint8
{
	Land,
	Water
};

/** 지형과 별도로 관리하는 셀 자원이다. 자원은 땅에만 배치한다. */
UENUM(BlueprintType)
enum class ECellResourceType : uint8
{
	None,
	Wood,
	Stone
};

USTRUCT(BlueprintType)
struct PROJECT_FACTORY_API FGridCellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	ETerrainType TerrainType = ETerrainType::Land;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	ECellResourceType ResourceType = ECellResourceType::None;
};

/** 셀은 행 우선 순서로 저장하며 인덱스는 Y * Width + X다. 생성 전에는 비어 있다. */
USTRUCT(BlueprintType)
struct PROJECT_FACTORY_API FTerrainChunkData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FIntPoint ChunkCoordinate = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "1024"))
	int32 Width = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "1024"))
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TArray<FGridCellData> Cells;
};
