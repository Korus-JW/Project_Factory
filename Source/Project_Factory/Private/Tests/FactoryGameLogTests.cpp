#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FactoryGameLog.h"
#include "Logging/FactoryLogComponent.h"
#include "Buildings/FactoryBuildingBase.h"
#include "Roads/RoadNetworkActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryGameLogTest, "ProjectFactory.Logging.FailureTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryGameLogTest::RunTest(const FString& Parameters)
{
	FFactoryOperationLog Log;
	// 같은 실패는 억제하고 원인 변경 및 성공 후 재실패는 다시 기록한다.
	AddExpectedMessagePlain(TEXT("Reason A"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
	AddExpectedMessagePlain(TEXT("Reason B"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("실패 결과 유지"), Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason A")));
	Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason A"));
	Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason B"));
	Log.Succeed(TEXT("TestOperation"));
	Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason A"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryLogComponentTest, "ProjectFactory.Logging.Component",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryLogComponentTest::RunTest(const FString& Parameters)
{
	auto* First = NewObject<UFactoryLogComponent>();
	auto* Second = NewObject<UFactoryLogComponent>();
	// 객체별 독립 기록과 성공 후 재실패, 원인 변경을 검증한다.
	AddExpectedMessagePlain(TEXT("Component reason A"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 3);
	AddExpectedMessagePlain(TEXT("Component reason B"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("컴포넌트 실패 반환"), First->ReportFailure(TEXT("Test"), TEXT("Component reason A")));
	First->ReportFailure(TEXT("Test"), TEXT("Component reason A"), TEXT("Changed details"));
	Second->ReportFailure(TEXT("Test"), TEXT("Component reason A"));
	First->ReportFailure(TEXT("Test"), TEXT("Component reason B"));
	First->ClearFailure(TEXT("Test"));
	First->ReportFailure(TEXT("Test"), TEXT("Component reason A"));

	AddExpectedMessagePlain(TEXT("Component warning message"), ELogVerbosity::Warning);
	AddExpectedMessagePlain(TEXT("Component error message"), ELogVerbosity::Error);
	First->WriteLog(TEXT("Component info message"));
	First->WriteLog(TEXT("Component warning message"), EFactoryLogLevel::Warning);
	First->WriteLog(TEXT("Component error message"), EFactoryLogLevel::Error);
	TestFalse(TEXT("틱 비활성화"), First->PrimaryComponentTick.bCanEverTick);
	TestNotNull(TEXT("건물 기본 로그 컴포넌트"), GetDefault<AFactoryBuildingBase>()->FactoryLog.Get());
	TestNotNull(TEXT("도로 기본 로그 컴포넌트"), GetDefault<ARoadNetworkActor>()->FactoryLog.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryLogLevelTest, "ProjectFactory.Logging.LevelsAndEscalation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryLogLevelTest::RunTest(const FString& Parameters)
{
	const ELogVerbosity::Type PreviousVerbosity = LogFactoryGame.GetVerbosity();
	LogFactoryGame.SetVerbosity(ELogVerbosity::Verbose);
	auto* Component = NewObject<UFactoryLogComponent>();
	// 자동 검사 뒤 같은 원인의 경고가 억제되지 않고 한 번 출력되는지 검증한다.
	AddExpectedMessagePlain(TEXT("Level transition reason"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	Component->ReportFailure(TEXT("LevelTest"), TEXT("Level transition reason"), FString(), EFactoryLogLevel::Verbose);
	Component->ReportFailure(TEXT("LevelTest"), TEXT("Level transition reason"), FString(), EFactoryLogLevel::Verbose);
	Component->ReportFailure(TEXT("LevelTest"), TEXT("Level transition reason"), FString(), EFactoryLogLevel::Warning);
	Component->ReportFailure(TEXT("LevelTest"), TEXT("Level transition reason"), FString(), EFactoryLogLevel::Warning);
	// 일반 로그는 자동화 프레임워크가 집계하지 않으므로 실제 로그 파일에서 횟수를 검증한다.
	Component->ReportFailure(TEXT("RequestTest"), TEXT("User request rejection"), FString(), EFactoryLogLevel::Log);
	Component->ReportFailure(TEXT("RequestTest"), TEXT("User request rejection"), FString(), EFactoryLogLevel::Log);
	// Verbose의 실제 출력은 에디터 통합 검증에서 로그 파일로 확인한다.
	Component->WriteLog(TEXT("Verbose component message"), EFactoryLogLevel::Verbose);
	LogFactoryGame.SetVerbosity(PreviousVerbosity);
	// 기존 블루프린트에 저장된 열거형 값의 호환성을 확인한다.
	TestEqual(TEXT("기존 Log 값"), uint8(EFactoryLogLevel::Log), uint8(0));
	TestEqual(TEXT("기존 Warning 값"), uint8(EFactoryLogLevel::Warning), uint8(1));
	TestEqual(TEXT("기존 Error 값"), uint8(EFactoryLogLevel::Error), uint8(2));
	return true;
}

#endif
