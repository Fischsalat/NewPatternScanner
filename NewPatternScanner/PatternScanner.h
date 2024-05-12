#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <cctype>


struct ConstexprError { mutable int Value; };

/* C++ does not provide any way of conditionally aborting constant evaluation. The only way to achieve this is by triggering undefined behavior. */
#define FailConstantEvaluation(ErrorMessageString) reinterpret_cast<const ConstexprError*>(ErrorMessageString)->Value = 0;


namespace PatternScannerImpl
{
	namespace ParserImpl
	{
		constexpr bool IsUppercaseHexDigit(char C) { return C >= 'A' && C <= 'F'; }
		constexpr bool IsLowercaseHexDigit(char C) { return C >= 'a' && C <= 'f'; }

		constexpr bool IsDecDigit(char C) { return C >= '0' && C <= '9'; }
		constexpr bool IsHexDigit(char C) { return IsUppercaseHexDigit(C) || IsLowercaseHexDigit(C); }

		constexpr uint8_t GetValueFromDecDigit(char C) { return C - '0'; }
		constexpr uint8_t GetValueFromHexDigit(char C) { return (IsUppercaseHexDigit(C) ? (C - 'A') : (C - 'a')) + 10; }

		/* Converts 1-9 and [a-f|A-F] to an unsigned integral value */
		constexpr uint8_t HexDigitToNumber(char C)
		{
			if (IsDecDigit(C))
				return GetValueFromDecDigit(C);

			if (IsHexDigit(C))
				return GetValueFromHexDigit(C);

			return 0xFF;
		}

		consteval bool IsFF(uint8_t Value)
		{
			return Value == 0xFF;
		}

		constexpr uint8_t ParseHexPair(char L, char R)
		{
			const uint8_t LeftValue = HexDigitToNumber(L);
			const uint8_t RigthValue = HexDigitToNumber(R);

			if constexpr (std::is_constant_evaluated())
			{
				if (LeftValue == 0xFF)
					FailConstantEvaluation("The left Value is invalid and couldn't be parsed.");

				if (RigthValue == 0xFF)
					FailConstantEvaluation("The right Value is invalid and couldn't be parsed.");
			}

			return (HexDigitToNumber(L) << 4) | HexDigitToNumber(R);
		}

		template<int32_t PatternStrLength>
		consteval void ParseStringToByteArray(const char(&PatternStr)[PatternStrLength], std::vector<int16_t>& ByteValues)
		{
			ByteValues.clear();

			for (int i = 1; i < PatternStrLength; i++)
			{
				const char CurrentChar = PatternStr[i-1];
			
				if ( CurrentChar == ' ')
					continue;
			
				if (CurrentChar == '?')
				{
					ByteValues.push_back(-1);
					continue;
				}
			
				/* Check if we're at the last valid index (excluding the null-terminator) */
				if (i == (PatternStrLength - 1))
					FailConstantEvaluation("Pattern must end with two digit hex number, space, or questionmark! Ended with single digit.");
			
				const char NextChar = PatternStr[i];
			
				if (NextChar == ' ' || NextChar == '?')
					FailConstantEvaluation("Single non-space and non-questionmark character encountered! Invalid!");
			
				ByteValues.push_back(ParseHexPair(CurrentChar, NextChar));

				// Increment before the loop-counter itereates to skip the next character as we're always parsing two at once (when it's hex and not a space/wildcard)
				i++;
			}
		}
	}


	template<int32_t PatternByteCount>
	struct PatternInfo
	{
	private:
		std::array<int16_t, PatternByteCount> PatternBytes;
	};

	inline bool IsInPattern(uint8_t Value)
	{
		return true;
	}

	inline uint16_t GetFirstOccurenceOfValueFromBack(uint8_t Value)
	{
		/* Return actual pos in the pattern, the check for if (IsInPattern(Value)) has been done already. */
		return 0x0;
	}

	/* Pseudo-implementation of the boyer-moore algorithm */
	inline const void* FindPattern(const uint8_t* Memory, int32_t SearchRange, int16_t* PatternBytes, int32_t PatternLength)
	{
		if (SearchRange <= 0x0)
			return nullptr;

		for (int i = PatternLength; i < SearchRange; i++)
		{
			for (int j = 0x0; j < PatternLength; j++)
			{
				const int32_t CurrentPatternIndex = (PatternLength - 1) - j;

				const int16_t CurrentPatternValue = PatternBytes[CurrentPatternIndex];
				const uint8_t CurrentInnerLoopValue = Memory[i - j];

				if (CurrentPatternValue == -1)
					continue;

				if (static_cast<uint8_t>(CurrentPatternValue) == CurrentInnerLoopValue)
				{
					if (j == (PatternLength - 1))
						return Memory + i - j;

					continue;
				}

				if (!IsInPattern(CurrentInnerLoopValue))
				{
					i += PatternLength;
					break;
				}

				i += GetFirstOccurenceOfValueFromBack(CurrentInnerLoopValue);
				break;
			}
		}
	}
}