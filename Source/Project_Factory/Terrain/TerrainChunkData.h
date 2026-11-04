#pragma once

#include "CoreMinimal.h"
#include "TerrainChunkData.generated.h"

UENUM(BlueprintType)
enum class ETerrainType : uint8
{
	Land,
	Water
};

USTRUCT(BlueprintType)
struct PROJECT_FACTORY_API FGridCellData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	ETerrainType TerrainType = ETerrainType::Land;
};

/** Row-major cells: Index = Y * Width + X. Empty by default until generated. */
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
