#include "FactoryBuildingBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "../Terrain/TerrainWorldSubsystem.h"

void AFactoryBuildingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTerrainWorldSubsystem* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this))
	{
		Terrain->UnregisterBuilding(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AFactoryBuildingBase::Destroyed()
{
	// 에디터에서 제거되는 경우에도 점유를 정리한다. 중복 해제 호출은 안전하다.
	if (UTerrainWorldSubsystem* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this))
	{
		Terrain->UnregisterBuilding(this);
	}
	Super::Destroyed();
}

AFactoryBuildingBase::AFactoryBuildingBase()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("BuildingRoot"));
	SetRootComponent(BuildingRoot);

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(BuildingRoot);
	BuildingMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	BuildingMesh->SetGenerateOverlapEvents(false);

	AllowedTerrainTypes.Add(ETerrainType::Land);
}

FIntPoint AFactoryBuildingBase::GetFootprintSize() const
{
	return FIntPoint(FMath::Clamp(FootprintSize.X, 1, 1024), FMath::Clamp(FootprintSize.Y, 1, 1024));
}

FIntPoint AFactoryBuildingBase::GetRotatedFootprintSize(int32 QuarterTurns) const
{
	const FIntPoint Size = GetFootprintSize();
	return QuarterTurns % 2 != 0 ? FIntPoint(Size.Y, Size.X) : Size;
}

TArray<FIntPoint> AFactoryBuildingBase::GetFootprintCells(FIntPoint AnchorGridCoordinate, int32 QuarterTurns) const
{
	const FIntPoint Size = GetRotatedFootprintSize(QuarterTurns);
	TArray<FIntPoint> Cells;
	// 좌표 덧셈으로 인한 정수 오버플로를 방지한다.
	if (static_cast<int64>(AnchorGridCoordinate.X) + Size.X - 1 > MAX_int32
		|| static_cast<int64>(AnchorGridCoordinate.Y) + Size.Y - 1 > MAX_int32)
	{
		return Cells;
	}
	Cells.Reserve(Size.X * Size.Y);
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			Cells.Emplace(AnchorGridCoordinate.X + X, AnchorGridCoordinate.Y + Y);
		}
	}
	return Cells;
}

bool AFactoryBuildingBase::CanPlaceOnTerrain(FIntPoint AnchorGridCoordinate, int32 QuarterTurns) const
{
	const UTerrainWorldSubsystem* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this);
	if (!Terrain)
	{
		return false;
	}
	const TArray<FIntPoint> Cells = GetFootprintCells(AnchorGridCoordinate, QuarterTurns);
	if (Cells.IsEmpty())
	{
		return false;
	}
	for (const FIntPoint& Coordinate : Cells)
	{
		FGridCellData Cell;
		if (!Terrain->GetCellAtGridCoordinate(Coordinate, Cell) || !IsTerrainAllowed(Cell.TerrainType))
		{
			return false;
		}
	}
	return true;
}

void AFactoryBuildingBase::UpdateCanBuild(bool bInCanBuild)
{
	bCanBuild = bInCanBuild;
	OnCanBuildChanged(bCanBuild);
}

bool AFactoryBuildingBase::IsTerrainAllowed(ETerrainType TerrainType) const
{
	return AllowedTerrainTypes.Contains(TerrainType);
}

void AFactoryBuildingBase::SetPlacementPreview(bool bPreview)
{
	if (bPlacementPreview == bPreview)
	{
		return;
	}

	if (bPreview)
	{
		bCollisionEnabledBeforePreview = GetActorEnableCollision();
		SetActorEnableCollision(false);
	}
	else
	{
		SetActorEnableCollision(bCollisionEnabledBeforePreview);
	}

	bPlacementPreview = bPreview;
	OnPlacementPreviewChanged(bPreview);
}

