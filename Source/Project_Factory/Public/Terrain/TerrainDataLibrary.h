#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TerrainChunkData.h"
#include "TerrainDataLibrary.generated.h"

UCLASS()
class PROJECT_FACTORY_API UTerrainDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 언리얼 센티미터 단위의 격자 간격이다. */
	static constexpr float CellSize = 500.0f;

	/** Y < WaterRows인 행은 물이며 땅의 약 10%씩 나무와 돌을 배치한다.
	 * 같은 입력은 같은 분포를 만든다. 크기는 1~1024이며 WaterRows는 범위 안으로 보정한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Terrain")
	static bool GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
		int32 Width = 10, int32 Height = 10, int32 WaterRows = 2);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static bool IsValidChunk(const FTerrainChunkData& Data);

	/** 데이터나 좌표가 유효하지 않으면 INDEX_NONE을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static int32 GetCellIndex(const FTerrainChunkData& Data, int32 X, int32 Y);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static bool GetCell(const FTerrainChunkData& Data, int32 X, int32 Y, FGridCellData& OutCell);

	/** 청크의 최소 XY 모서리를 기준으로 한 셀 중심이다. 메시 피벗은 중심에 둔다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static FVector GetCellLocalCenter(int32 X, int32 Y);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static FVector GetCellLocalOrigin(int32 X, int32 Y);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static float GetCellSize() { return CellSize; }


};
