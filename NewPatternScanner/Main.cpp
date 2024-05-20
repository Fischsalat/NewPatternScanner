#include "PatternScanner.h"
#include <iostream>
#include <initializer_list>

template<typename T>
consteval T MakeValidArrayNum(T Value)
{
	static_assert(std::is_integral_v<T>, "Value must be of integral type!");

	return Value > 0x100 ? 0x100 : Value < 0x1 ? 0x1 : Value;
}

template<int32_t NumResultEntries, int32_t PatternStrLength>
consteval std::array<int16_t, MakeValidArrayNum(NumResultEntries)> TestPattern(const char(&PatternStr)[PatternStrLength], const std::initializer_list<int16_t>& ExptectedValues)
{
	std::vector<int16_t> ResultBytes;
	PatternScannerImpl::ParserImpl::ParseStringToByteArray(PatternStr, ResultBytes);

	if (ResultBytes.size() != NumResultEntries)
		FailConstantEvaluation("Parsing the pattern didn't work correctly, the ammount of values in the output vector isn't as expected");

	if (ResultBytes != std::vector<int16_t>(ExptectedValues))
		FailConstantEvaluation("Parsing the pattern didn't work correctly, the resulting values don't match the expected values!");
	
	std::array<int16_t, MakeValidArrayNum(NumResultEntries)> RetArrayForDebug = {};

	for (int i = 0; i < RetArrayForDebug.size(); i++)
		RetArrayForDebug[i] = i < ResultBytes.size() ? ResultBytes[i] : 0x1111;

	return RetArrayForDebug;
}

inline consteval auto GetPatternInfo()
{
	const std::vector<int16_t> Vec = { 0x48, 0x8B, -1, 0x48, 0xC1 };
	PatternScannerImpl::PatternInfo<5> Data(Vec);

	return Data;
}

int32_t TestFindPattern(const std::initializer_list<int16_t>& Values = { 0x48, 0x8B, -1, -1, 0x48, 0xC1 })
{
	//const std::vector<int16_t> Vec = Values;
	//PatternScannerImpl::PatternInfo<5> Data(Vec);

	uint8_t FakeMemory[] = {
		0x48, 0x8B, 0x15, 0x81, 0x4B, 0x0F, 0x00,
		0x48, 0x8B, 0xC1,
		0x48, 0xC1, 0xE8, 0x09,
		0x48, 0x8B, 0x14, 0xC2,
		0x48, 0x8B, 0xC1,
		0x48, 0xC1, 0xE8, 0x03,
		0xF6, 0xC1, 0x0F,
		0x75, 0x07,
		0x48, 0x0F, 0xA3, 0xC2,
		0x73, 0x0C,
		0xC3
	};

	int32_t Result = PatternScannerImpl::FindPattern<true>(FakeMemory, sizeof(FakeMemory), GetPatternInfo());

	//const bool b = Result < (void*)30;

	return Result;
}


inline bool TestFindSingleBytePattern()
{
	std::cout << std::hex << "TestFindPattern(): " << TestFindPattern() << std::endl;

	//static_assert(Test(0xAA) == 0x3, "Third of { 0xAA3 }, should be { 0x3 }");
	//static_assert(Test(0xAA) == 0x2, "Third of { 0xAA2 }, should be { 0x2 }");
	//static_assert(Test(0xAA) == 0x1, "Third of { 0xAA1 }, should be { 0x1 }");
	//static_assert(Test(0xAA) == 0xFF, "Third of { 0xAA1 }, should be { 0xFF }");
	//static_assert(Test(0xAA) == 0x0, "Third of { 0xAA }, should be { 0x0 }");
	//static_assert(Test(0xBB) == 0x0, "Third of { 0xBB }, should be { 0x0 }");
	//static_assert(Test(0xCC) == 0x0, "Third of { 0xCC }, should be { 0x1 }");
	//static_assert(Test(0xDD) == 0x0, "Third of { 0xDD }, should be { 0x1 }");
	//static_assert(Test(0xEE) == 0x0, "Third of { 0xEE }, should be { 0x2 }");
	//static_assert(Test(0xFF) == 0x0, "Third of { 0xFF }, should be { 0x2 }");

	return false;
}

int main()
{
	PatternScannerImpl::ValueToUnsingedIntegralType<0xFFFFFFFFFFFFFFFF> Varr = 30;
	PatternScannerImpl::ValueToUnsingedIntegralType<sizeof(float) != 6> Varr3 = 30;

	std::cout << "sizeof(Varr): " << sizeof(Varr) << std::endl;

	constexpr auto FirstTest = TestPattern<0x5>("48 8B ? ? E8", { 0x48, 0x8B, -1, -1, 0xE8 });
	constexpr auto SecondTest = TestPattern<0x1>("48", { 0x48 });
	constexpr auto ThirdTest = TestPattern<0x0>("", { });
	constexpr auto FourthTest = TestPattern<0x99>("40 55 56 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 8D 7C 24 ? B9 ? ? ? ? B8 ? ? ? ? F3 AB 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8 ? ? ? ? 66 89 45 ? B8", { 0x40, 0x55, 0x56, 0x57, 0x48, 0x81, 0xEC, -1, -1, -1, -1, 0x48, 0x8D, 0x6C, 0x24, -1, 0x48, 0x8D, 0x7C, 0x24, -1, 0xB9, -1, -1, -1, -1, 0xB8, -1, -1, -1, -1, 0xF3, 0xAB, 0x48, 0x8B, 0x05, -1, -1, -1, -1, 0x48, 0x33, 0xC5, 0x48, 0x89, 0x85, -1, -1, -1, -1, 0x48, 0x8D, 0x0D, -1, -1, -1, -1, 0xE8, -1, -1, -1, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8, -1, -1, -1, -1, 0x66, 0x89, 0x45, -1, 0xB8 });

	for (int i = 0; i < FirstTest.size(); i++)
		std::cout << std::hex << FirstTest[i] << std::endl;

	for (int16_t Value : { 0x48, 0x8B, -1, -1, 0xE8 })
		std::cout << std::hex << Value << std::endl;

	TestFindSingleBytePattern();

	return 0;
}