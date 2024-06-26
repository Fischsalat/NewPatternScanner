#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <cctype>

/* Debugging */
#include <iostream>

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

		/*
		* Pattern Syntax: "48 8B 50|60 ? ? [49 ? B8 | 48 8B] 00 [?2] 40 [?1-4] 00 00 {0} {1} 22"
		* 
		* Wildcard: "?" -> Any arbitrary byte value
		* 
		* Toggle:
		*	* ShortToggle: "49|59" -> Byte can either be 0x49 or 0x59. Size is 1 byte on both sides.
		*	* LongToggle: "[49 8B ? | ? 48] 50 70" -> At this point in the pattern there should either occure "49 8B ?", or "? 48", before the pattern continues normally with "50 70"
		* 
		* Omissions: 
		*	* Single-Byte: "50 [?] 48" -> Allows for a single extra **optional** byte inbetween 50 and 48. Will match both "50 48" as well as "50 n 48" where n is any byte value.
		*	* N-Byte: "50 [?2] 48" (N = 2) -> Allows for N (=2) extra **optional** bytes inbetween 50 and 48. Will match "50 48" as well as "50 n m 48" where n, m are byte values.
		*	* N-M-Range: "50 [?1-3] 48" (N=1, M=3) -> Allows for N to M random bytes to appear in the pattern. 
		*		* Will **NOT** match any ammount of omitted bytes outside of the range [N, M]. Will not match "50 48", as 0 is outside of the range.
		*		* Will match "50 X1 48", "50 X1 X2 48", "50 X1 X2 X3 48", where X1, X2, X3 are byte values.
		*		* Will not match "50, X1, X2, X3, X4 48", as upper limit M=4 is out of range.
		* 
		* Runtime-Value:
		*	* Indicates that the byte at this position is not known at compile-time and is later supplied as a runtime-value
		*	* "{0} 49" indicates that the first byte is supposed to be replaced by the first user-supplied byte value. If the first value is 0x33 the pattern searched will be "33 49"
		*   * The maximum number of runtime-supplied values is 16 (0 - F)
		* 
		* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		* Implemenation Notes:
		*	* Constexpr evaluate used featuers and templatize pattern-scanner implementation based on the used features to avoid checking for things like Omissions if there aren't any
		*	*
		*	* struct ShortToggle { }; 
		*	* struct LongToggle { NumBytesLeft, NumBytesRight }; 
		* 	*
		* 	* struct SingleByteOmission { }; 
		* 	* struct NByteOmission { NumOptionalBytes }; 
		* 	* struct NMByteOmission { MinNumOptionalBytes, MaxNumOptionalBytes }; 
		* 	*
		* 	* struct RuntimeValue { }; 
		*/
		template<int32_t PatternStrLength>
		consteval int32_t ValidatePatternAndGetByteLength(const char(&PatternStr)[PatternStrLength])
		{
			bool bLastCharWasHexDigit = true;

			/* Handeling Toggles/Omissions */
			bool bAreSquareBracketsOpen = false;

			/* Hnadling Togles */
			bool bEncounteredIdkThisVariableIsUnfinishedImStillWorkingStuffOutRightNow;

			/* Handeling RuntimeValues {0} {1} ... {F} */
			bool bAreBracesOpen = false;
			bool bEncounteredSingleDigit = false;




			for (int i = 0; i < PatternStrLength; i++)
			{
				const char C = PatternStr[i];

				/* Ignore spaces */
				if (C == ' ')
					continue;

				//if ()
			}

			return 0;
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

	template<typename LT, typename RT>
	inline constexpr LT Min(LT L, RT R)
	{
		static_assert(std::is_integral_v<LT> && std::is_integral_v<RT>, "Type must be of integral value!");

		return L < R ? L : R;
	}

	template<typename T>
	inline constexpr T Max(T L, T R)
	{
		static_assert(std::is_integral_v<T>, "Type must be of integral value!");

		return L > R ? L : R;
	}

	inline constexpr uint8_t ValueToByteWidth(uint64_t Value)
	{
		if (Value > std::numeric_limits<uint32_t>::max())
		{
			return 0x8;
		}
		else if (Value > std::numeric_limits<uint16_t>::max())
		{
			return 0x4;
		}
		else if (Value > std::numeric_limits<uint8_t>::max())
		{
			return 0x2;
		}

		return 0x1;
	}

	template<uint8_t Size>
	struct SelectUnsignedIntegralTypeBySize
	{
		static_assert(Size > 8, "The max ammount of byte-width supported is 8!");
		static_assert(Size == 3 || Size == 6 || Size == 7, "The byte-width must be 1; 2; 4; 8; but was 3; 6; 7!");
	};

	template<>
	struct SelectUnsignedIntegralTypeBySize<1> { using Type = uint8_t; };

	template<>
	struct SelectUnsignedIntegralTypeBySize<2> { using Type = uint16_t; };

	template<>
	struct SelectUnsignedIntegralTypeBySize<4> { using Type = uint32_t; };

	template<>
	struct SelectUnsignedIntegralTypeBySize<8> { using Type = uint64_t; };

	template<uint64_t Value>
	using ValueToUnsingedIntegralType = SelectUnsignedIntegralTypeBySize<ValueToByteWidth(Value)>::Type;

	/* Precomputed data to perform a patter-search using the Knuth-Morris-Pratt algortihm */
	template<int32_t PatternByteCount>
	struct PatternInfo
	{
	private:
		static constexpr int16_t Wildcard = -1;
	private:
		/* Bytes of the pattern in the range [0, 255] and -1 representing wildcards */
		int16_t PatternBytes[PatternByteCount] = { 0x0 };

		/* The partial match table from the Knuth-Morris-Pratt algortihm */
		uint16_t PartialMatchTable[PatternByteCount] = { 0x0 };
		
	public:
		inline consteval PatternInfo(const std::vector<int16_t>& InBytes)
		{
			std::copy_n(InBytes.data(), PatternScannerImpl::Min(PatternByteCount, InBytes.size()), PatternBytes);
			BuildPartialMatchTable(InBytes);
		}

	private:
		static consteval int32_t MemcmpWithWildCard(const int16_t* L, const int16_t* R, int32_t Length)
		{
			for (int i = 0; i < Length; i++)
			{
				if (L[i] == Wildcard || R[i] == Wildcard)
					continue;

				if (L[i] != R[i])
					return i;
			}

			return 0;
		}

		inline consteval void BuildPartialMatchTable(const std::vector<int16_t>& InPatternBytes)
		{
			/* Default initialize to the index so we don't skip bytes if we're not allowed to. */
			for (int i = 0; i < PatternByteCount; i++)
				PartialMatchTable[i] = i;

			/* Start at 1 because the PMT entry for index 0 is always 0. */
			for (int i = 1; i < PatternByteCount; i++)
			{
				for (int j = i; j > 0; j--)
				{
					const int16_t* PrefixStart = PatternBytes + i - j;
					const int16_t* PostfixStart = PatternBytes + j - 1;

					const int32_t ComparisonLength = j;

					if (MemcmpWithWildCard(PrefixStart, PostfixStart, ComparisonLength) == 0)
					{
						PartialMatchTable[i] = ComparisonLength;
						break;
					}
				}
			}
		}

	public:
		inline constexpr int32_t GetLength() const
		{
			return PatternByteCount; 
		}

		inline constexpr int16_t GetPatternValue(size_t Index) const
		{
			if (Index < 0 || Index >= GetLength())
				return 0xCDCD;

			return PatternBytes[Index];
		}

		inline constexpr int16_t GetPartialMatchTableEntry(size_t Index) const
		{
			if (Index < 0 || Index >= GetLength())
				return 0x0;

			return PartialMatchTable[Index];
		}

	public:
		inline constexpr int16_t operator[](size_t Index) const
		{
			return GetPatternValue(Index);
		}
	};

	consteval std::vector<int16_t> GetVector(const std::vector<char>& Str)
	{
		std::vector<int16_t> PatternBytes;
		//ParserImpl::ParseStringToByteArray(PatternStr2, PatternBytes);

		for (int i = 0; Str[i] != '\n'; i++)
			PatternBytes[i] = 7;

		return PatternBytes;
	}

	template<int32_t PatternStrLength>
	consteval decltype(auto) CreatePattnerInfo(const char(&PatternStr)[PatternStrLength])
	{
		std::vector<char> Str(PatternStrLength);

		for (int i = 0; i < PatternStrLength; i++)
			Str.push_back(PatternStr[i]);

		//constexpr auto PatternBytes = GetVector(Str);
		//
		//return PatternInfo<PatternBytes.size()>(PatternBytes);
		return 1;
	}

	/* Pseudo-implementation of the Knuth-Morris-Pratt algorithm */
	template<bool bIsTest = false, int32_t PatternLengthBytes>
	inline std::conditional_t<bIsTest, int32_t, void*> FindPattern(uint8_t* Memory, int32_t SearchRange, const PatternInfo<PatternLengthBytes>& Pattern)
	{
		if (SearchRange <= 0x0)
			return NULL;

		const int32_t PatternMaxIndex = Pattern.GetLength() - 1;
		const int32_t SearchEnd = SearchRange - Pattern.GetLength();

		for (int CurrentMemPos = 0; CurrentMemPos < SearchEnd; CurrentMemPos++)
		{
			if (CurrentMemPos == 0xE)
				float a = 30;

			std::cout << "Mem[" << CurrentMemPos  << "]: " << "0x" << std::hex << +Memory[CurrentMemPos] << std::endl;

			for (int MatchCount = 0x0; MatchCount < Pattern.GetLength(); MatchCount++)
			{
				const int32_t CurrentPatternIndex = PatternMaxIndex - MatchCount;

				const int16_t CurrentPatternValue = Pattern[CurrentPatternIndex];
				const uint8_t CurrentMemoryValue = Memory[CurrentMemPos + MatchCount];

				/* Wildcard, skip this byte */
				if (CurrentPatternValue == -1)
					continue;

				if (static_cast<uint8_t>(CurrentPatternValue) == CurrentMemoryValue)
				{
					if (MatchCount == PatternMaxIndex)
					{
						if constexpr (bIsTest)
						{
							return CurrentMemPos;
						}
						else
						{
							return Memory + CurrentMemPos;
						}
					}

					continue;
				}

				CurrentMemPos += Max(0, (MatchCount - Pattern.GetPartialMatchTableEntry(MatchCount)));
				break;
			}
		}

		return NULL;
	}
}