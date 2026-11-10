#include "Terrain/TerrainDataLibrary.h"

namespace
{
	constexpr int32 MaxChunkDimension = 1024;

	bool AreDimensionsValid(int32 Width, int32 Height)
	{
		return Width > 0 && Height > 0 && Width <= MaxChunkDimension && Height <= MaxChunkDimension;
	}
}

bool UTerrainDataLibrary::GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
	int32 Width, int32 Height, int32 WaterRows)
{
	OutData = FTerrainChunkData();
	OutData.ChunkCoordinate = ChunkCoordinate;
	if (!AreDimensionsValid(Width, Height))
	{
		return false;
	}

	OutData.Width = Width;
	OutData.Height = Height;
	OutData.Cells.SetNum(Width * Height);
	const int32 ClampedWaterRows = FMath::Clamp(WaterRows, 0, Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			OutData.Cells[Y * Width + X].TerrainType =
				Y < ClampedWaterRows ? ETerrainType::Water : ETerrainType::Land;
		}
	}
	return true;
}

bool UTerrainDataLibrary::IsValidChunk(const FTerrainChunkData& Data)
{
	return AreDimensionsValid(Data.Width, Data.Height)
		&& Data.Cells.Num() == Data.Width * Data.Height;
}

int32 UTerrainDataLibrary::GetCellIndex(const FTerrainChunkData& Data, int32 X, int32 Y)
{
	if (!IsValidChunk(Data) || X < 0 || Y < 0 || X >= Data.Width || Y >= Data.Height)
	{
		return INDEX_NONE;
	}
	return Y * Data.Width + X;
}

bool UTerrainDataLibrary::GetCell(const FTerrainChunkData& Data, int32 X, int32 Y, FGridCellData& OutCell)
{
	OutCell = FGridCellData();
	const int32 Index = GetCellIndex(Data, X, Y);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutCell = Data.Cells[Index];
	return true;
}

FVector UTerrainDataLibrary::GetCellLocalCenter(int32 X, int32 Y)
{
	return FVector((static_cast<double>(X) + 0.5) * CellSize,
		(static_cast<double>(Y) + 0.5) * CellSize, 0.0);
}

FVector UTerrainDataLibrary::GetCellLocalOrigin(int32 X, int32 Y)
{
	return FVector(static_cast<double>(X) * CellSize,
		static_cast<double>(Y) * CellSize, 0.0);
}
