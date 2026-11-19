#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FactoryProductionComponent.h"
#include "Inventory/FactoryInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryRecipeSelectionTest, "ProjectFactory.Production.RecipeSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryRecipeSelectionTest::RunTest(const FString& Parameters)
{
	auto* Component = NewObject<UFactoryProductionComponent>();
	auto* Inventory = NewObject<UFactoryInventoryComponent>();
	Component->Inventory = Inventory;
	auto* Table = NewObject<UDataTable>();
	Table->RowStruct = FFactoryRecipeDefinition::StaticStruct();
	FFactoryItemAmount Item;
	Item.ItemId = TEXT("Stone"); Item.Quantity = 1;
	FFactoryRecipeDefinition Recipe;
	Recipe.Outputs = { Item };
	Table->AddRow(TEXT("Stone"), Recipe);
	Recipe.Outputs[0].ItemId = TEXT("Wood");
	Table->AddRow(TEXT("Wood"), Recipe);
	FDataTableRowHandle Stone;
	Stone.DataTable = Table; Stone.RowName = TEXT("Stone");
	FDataTableRowHandle Wood = Stone;
	Wood.RowName = TEXT("Wood");
	Component->AvailableRecipes = { Stone, Wood };
	Inventory->SetCapacity(EFactoryInventorySlot::Output, TEXT("Stone"), 1);
	Inventory->SetCapacity(EFactoryInventorySlot::Output, TEXT("Wood"), 1);
	TestTrue(TEXT("허용 제작법 선택"), Component->SelectRecipe(Stone));
	Component->AdvanceProduction(0.5f);
	TestTrue(TEXT("생산 중 변경 예약"), Component->SelectRecipe(Wood));
	TestEqual(TEXT("현재 작업 유지"), Component->GetSelectedRecipe().RowName, Stone.RowName);
	TestTrue(TEXT("현재 제작법 재선택"), Component->SelectRecipe(Stone));
	TestTrue(TEXT("변경 예약 취소"), Component->GetPendingRecipe().RowName.IsNone());
	Component->SelectRecipe(Wood);
	FDataTableRowHandle Invalid = Wood;
	Invalid.RowName = TEXT("Unknown");
	TestFalse(TEXT("목록 외 선택 거부"), Component->SelectRecipe(Invalid));
	TestEqual(TEXT("잘못된 요청 후 예약 유지"), Component->GetPendingRecipe().RowName, Wood.RowName);
	Inventory->SetCapacity(EFactoryInventorySlot::Output, TEXT("Stone"), 0);
	Component->AdvanceProduction(0.5f);
	TestEqual(TEXT("입고 대기 중 기존 제작법 유지"), Component->GetSelectedRecipe().RowName, Stone.RowName);
	Inventory->SetCapacity(EFactoryInventorySlot::Output, TEXT("Stone"), 1);
	Component->AdvanceProduction(0.0f);
	TestEqual(TEXT("입고 완료 후 전환"), Component->GetSelectedRecipe().RowName, Wood.RowName);
	Component->AdvanceProduction(1.0f);
	TestEqual(TEXT("새 제작법 생산"), Inventory->GetQuantity(EFactoryInventorySlot::Output, TEXT("Wood")), 1);
	TestEqual(TEXT("이전 완제품 보존"), Inventory->GetQuantity(EFactoryInventorySlot::Output, TEXT("Stone")), 1);
	return true;
}
#endif
