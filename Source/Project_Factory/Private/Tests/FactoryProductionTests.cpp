#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FactoryProductionComponent.h"
#include "Inventory/FactoryInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryProductionTest, "ProjectFactory.Production.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryProductionTest::RunTest(const FString& Parameters)
{
	auto* Production = NewObject<UFactoryProductionComponent>();
	auto* Inventory = NewObject<UFactoryInventoryComponent>();
	Production->Inventory = Inventory;
	const auto Input = EFactoryInventorySlot::Input;
	const auto Output = EFactoryInventorySlot::Output;
	FFactoryItemAmount Stone;
	Stone.ItemId = TEXT("Stone"); Stone.Quantity = 2;
	FFactoryItemAmount Slab;
	Slab.ItemId = TEXT("Slab"); Slab.Quantity = 1;
	FFactoryRecipeDefinition Recipe;
	Recipe.Inputs = { Stone }; Recipe.Outputs = { Slab }; Recipe.ProductionTimeSeconds = 2.0f;
	Inventory->SetCapacity(Input, Stone.ItemId, 10);
	Inventory->SetCapacity(Output, Slab.ItemId, 1);
	TestTrue(TEXT("제작법 설정"), Production->SetRecipe(Recipe));
	Production->AdvanceProduction(1.0f);
	TestTrue(TEXT("재료 대기"), Production->GetProductionState() == EFactoryProductionState::WaitingForMaterials);
	Inventory->AddItems(Input, Stone.ItemId, 4);
	Production->AdvanceProduction(1.0f);
	TestEqual(TEXT("시작 시 재료 소비"), Inventory->GetQuantity(Input, Stone.ItemId), 2);
	TestEqual(TEXT("진행률"), Production->GetProgress(), 0.5f);
	TestFalse(TEXT("작업 중 제작법 변경 차단"), Production->SetRecipe(Recipe));
	Production->SetProductionEnabled(false);
	Production->AdvanceProduction(10.0f);
	TestEqual(TEXT("일시정지 시간 보존"), Production->GetProgress(), 0.5f);
	Production->SetProductionEnabled(true);
	Inventory->SetCapacity(Output, Slab.ItemId, 0);
	Production->AdvanceProduction(1.0f);
	TestEqual(TEXT("공간 부족 완성품 보존"), Production->GetPendingOutputs().Num(), 1);
	TestEqual(TEXT("다음 작업 재료 보존"), Inventory->GetQuantity(Input, Stone.ItemId), 2);
	Inventory->SetCapacity(Output, Slab.ItemId, 1);
	Production->AdvanceProduction(0.0f);
	TestEqual(TEXT("공간 회복 후 완성품 입고"), Inventory->GetQuantity(Output, Slab.ItemId), 1);
	Production->AdvanceProduction(5.0f);
	TestEqual(TEXT("출력 가득 차면 재료 미소비"), Inventory->GetQuantity(Input, Stone.ItemId), 2);
	Inventory->RemoveItems(Output, Slab.ItemId, 1);
	Recipe.Inputs.Reset();
	Recipe.Outputs = { Slab, Slab };
	Inventory->SetCapacity(Output, Slab.ItemId, 10);
	TestTrue(TEXT("채취 및 중복 산출물 제작법"), Production->SetRecipe(Recipe));
	Production->AdvanceProduction(4.0f);
	TestEqual(TEXT("긴 프레임 두 주기 생산"), Inventory->GetQuantity(Output, Slab.ItemId), 4);
	Recipe.ProductionTimeSeconds = 0.0f;
	TestFalse(TEXT("잘못된 제작 시간 거부"), Production->SetRecipe(Recipe));
	return true;
}
#endif
