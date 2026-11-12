#include "Buildings/FactoryBuildingBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Terrain/TerrainWorldSubsystem.h"
#include "Terrain/TerrainDataLibrary.h"
#include "Engine/StaticMesh.h"

bool AFactoryBuildingBase::GetRoadPort(bool bEntrance, FIntPoint AnchorGridCoordinate,
	int32 QuarterTurns, FIntPoint& OutRoadCell, int32& OutConnectionMask) const
{
	OutRoadCell = FIntPoint::ZeroValue;
	OutConnectionMask = 0;
	const FBuildingRoadPort& Port = bEntrance ? EntrancePort : ExitPort;
	FIntPoint Size = GetFootprintSize();
	FIntPoint Cell = Port.LocalCell;
	int32 Direction = static_cast<int32>(Port.OutwardDirection);
	if (!Port.bEnabled || Direction > 3 || Cell.X < 0 || Cell.Y < 0 || Cell.X >= Size.X || Cell.Y >= Size.Y) return false;
	const FIntPoint Steps[] = { FIntPoint(0,1), FIntPoint(1,0), FIntPoint(0,-1), FIntPoint(-1,0) };
	const FIntPoint Outside = Cell + Steps[Direction];
	// 내부를 향하는 연결점은 허용하지 않는다.
	if (Outside.X >= 0 && Outside.Y >= 0 && Outside.X < Size.X && Outside.Y < Size.Y) return false;
	const int32 Turns = (QuarterTurns % 4 + 4) % 4;
	for (int32 I = 0; I < Turns; ++I)
	{
		// 양의 Yaw로 회전한 뒤 새 점유 사각형의 최소 좌표를 원점으로 맞춘다.
		Cell = FIntPoint(Size.Y - 1 - Cell.Y, Cell.X);
		Size = FIntPoint(Size.Y, Size.X);
		Direction = (Direction + 3) % 4;
	}
	const int64 X = int64(AnchorGridCoordinate.X) + Cell.X + Steps[Direction].X;
	const int64 Y = int64(AnchorGridCoordinate.Y) + Cell.Y + Steps[Direction].Y;
	if (X < MIN_int32 || X > MAX_int32 || Y < MIN_int32 || Y > MAX_int32) return false;
	OutRoadCell = FIntPoint(int32(X), int32(Y));
	OutConnectionMask = 1 << ((Direction + 2) % 4);
	return true;
}

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

	RotationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RotationRoot"));
	RotationRoot->SetupAttachment(BuildingRoot);

	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
	BuildingMesh->SetupAttachment(RotationRoot);
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

void AFactoryBuildingBase::RotateBuilding()
{
	SetRotationStep((RotationStep % 4 + 1) % 4);
}

void AFactoryBuildingBase::SetRotationStep(int32 NewRotationStep)
{
	RotationStep = ((NewRotationStep % 4) + 4) % 4;
	RefreshBuildingTransform();
}

void AFactoryBuildingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshBuildingTransform();
}

void AFactoryBuildingBase::RefreshBuildingTransform()
{
	RotationStep = ((RotationStep % 4) + 4) % 4;
	const FIntPoint Size = GetRotatedFootprintSize(RotationStep);
	const double HalfCell = UTerrainDataLibrary::CellSize * 0.5;
	RotationRoot->SetRelativeLocation(FVector(Size.X * HalfCell, Size.Y * HalfCell, 0.0));
	RotationRoot->SetRelativeRotation(FRotator(0.0, RotationStep * 90.0, 0.0));

	// 기존 블루프린트에 저장된 이전 부모 연결도 새 회전 중심으로 연결한다.
	if (BuildingMesh->GetAttachParent() != RotationRoot)
	{
		BuildingMesh->AttachToComponent(RotationRoot, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (const UStaticMesh* Mesh = BuildingMesh->GetStaticMesh())
	{
		// 피벗 방향에 관계없이 메시 경계의 중심을 맞추며 원래 회전과 스케일은 보존한다.
		const FVector Center = BuildingMesh->GetRelativeRotation().RotateVector(
			Mesh->GetBoundingBox().GetCenter() * BuildingMesh->GetRelativeScale3D());
		FVector Location = BuildingMesh->GetRelativeLocation();
		Location.X = -Center.X + MeshCenterOffset.X;
		Location.Y = -Center.Y + MeshCenterOffset.Y;
		BuildingMesh->SetRelativeLocation(Location);
	}
}

