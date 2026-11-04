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
	/** Grid spacing in Unreal centimeters. */
	static constexpr float CellSize = 500.0f;

	/** Generates a flat test chunk. Rows with Y < WaterRows are water.
	 * Dimensions outside 1..1024 fail with empty output; WaterRows is clamped. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Terrain")
	static bool GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
		int32 Width = 10, int32 Height = 10, int32 WaterRows = 2);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static bool IsValidChunk(const FTerrainChunkData& Data);

	/** Returns INDEX_NONE for invalid data or coordinates. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static int32 GetCellIndex(const FTerrainChunkData& Data, int32 X, int32 Y);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static bool GetCell(const FTerrainChunkData& Data, int32 X, int32 Y, FGridCellData& OutCell);

	/** Local center relative to the chunk's lower XY corner; mesh pivot must be centered. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static FVector GetCellLocalCenter(int32 X, int32 Y);

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	static float GetCellSize() { return CellSize; }
};
