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

	/* A struct wrapping the 2-bit information stored by PatternInfo::OccurenceMap */
	struct ByteInfo
	{
	private:
		/* Data available for this byte */
		uint8_t DataByte : 2;

		/* Reserved bits used by other entries in the OccurenceMap */
		uint8_t RESERVED : 6;

		/* DataByte stores the number of thirds of a pattern that can be skipped for this byte */
		int32_t ThirdOfPatternLength;

	public:
		constexpr ByteInfo(uint8_t ByteData, int32_t PatternLength)
			: DataByte(ByteData), RESERVED(0), ThirdOfPatternLength(PatternLength / 3)
		{
		}

	private:
		inline constexpr uint8_t GetThirdsToSkip() const
		{
			/* 0b00 is reserved for "Byte is not in pattern" */
			return DataByte - 1;
		}

	public:
		inline constexpr bool IsInPattern() const
		{
			return DataByte > 0x0;
		}

		inline constexpr uint8_t GetByteSkipCount() const
		{
			return GetThirdsToSkip() * ThirdOfPatternLength;
		}
	};

	template<int32_t PatternByteCount, int32_t ByteCountWithoutWildcards>
	struct PatternInfo
	{
	private:
		enum PatternThird
		{
			NoOccurence = 0b00,

			OneThird = 0b01,
			TwoThrids = 0b10,
			ThreeThrids = 0b11
		};
	private:
		/* Number of bytes required to allocate Bitmap with 2-bits per byte */
		static constexpr uint8_t NumBytesForBitmap = 64;

		/* Third of the pattern length, used for OccurenceMap setup */
		static constexpr int32_t ThirdOfPatternLength = ByteCountWithoutWildcards / 3;

	private:
		static constexpr uint8_t TwoBitsSet = 0b11;

	private:
		/* Bytes of the pattern in the range [0, 255] and -1 representing wildcards */
		int16_t PatternBytes[PatternByteCount] = { 0x0 };

		/* Bitmap, of 2-bits per byte-value, describing how far from the back a byte first occures [00 -> not in pattern; 01 -> first third; 10 -> second third; 11 -> third third]*/
		uint8_t OccurenceMap[NumBytesForBitmap] = { 0x0 };
		
	public:
		inline consteval PatternInfo(const std::vector<int16_t>& InBytes)
		{
			std::copy_n(InBytes.data(), PatternScannerImpl::Min(PatternByteCount, InBytes.size()), PatternBytes);
			InitializeOccurenceMap(InBytes);
		}

	private:
		inline consteval void InitializeOccurenceMap(const std::vector<int16_t>& InPatternBytes)
		{
			constexpr int32_t TwoThirdsOfLength = (ByteCountWithoutWildcards * 2) / 3;

			int NonMaskByteIndexPlusOne = InPatternBytes.size();
			for (int i = InPatternBytes.size() - 1; i >= 0; --i)
			{
				const int16_t CurrentPatternByte = InPatternBytes[i];

				/* We don't want to do anything with wildcards */
				if (CurrentPatternByte == -1)
					continue;

				NonMaskByteIndexPlusOne--;

				/* Marker for which thrid of the pattern the byte first occures, starting at the back */
				const PatternThird CurrentThirdMarker = NonMaskByteIndexPlusOne >= TwoThirdsOfLength ? OneThird : NonMaskByteIndexPlusOne >= ThirdOfPatternLength ? TwoThrids : OneThird;

				uint8_t ByteOccurenceInfo = GetEntryFromOccurenceMap(static_cast<const uint8_t>(CurrentPatternByte));

				if (ByteOccurenceInfo == static_cast<uint8_t>(PatternThird::NoOccurence))
					SetEntryInOccurenceMap(static_cast<const uint8_t>(CurrentPatternByte), CurrentThirdMarker);
			}
		}

	private:
		inline constexpr uint8_t GetEntryFromOccurenceMap(uint8_t Index) const
		{
			const uint8_t ByteIndex = Index / 4;
			const uint8_t ShiftCount = (Index & TwoBitsSet) << 1;
			const uint8_t BitMask = TwoBitsSet << ShiftCount;

			return (OccurenceMap[ByteIndex] & BitMask) >> ShiftCount;
		}

		inline constexpr void SetEntryInOccurenceMap(uint8_t Index, PatternThird CurrentThird)
		{
			const uint8_t ByteIndex = Index / 4;
			const uint8_t ShiftCount = (Index & TwoBitsSet) << 1;
			const uint8_t BitMask = TwoBitsSet << ShiftCount;

			const uint8_t SetMask = CurrentThird << ShiftCount;

			/* Get a ref to the byte, clear the entry-value and set the new one. */
			uint8_t& ByteRef = OccurenceMap[ByteIndex];
			ByteRef &= ~BitMask;
			ByteRef |= SetMask;
		}

	public:
		inline constexpr ByteInfo GetByteInfo(uint8_t Index) const
		{
			return ByteInfo(GetEntryFromOccurenceMap(Index), GetLength());
		}

		inline constexpr int32_t GetLength() const
		{
			return PatternByteCount; 
		}

		inline constexpr int32_t GetLengthWithoutWildcards() const
		{
			return ByteCountWithoutWildcards;
		}

	public:
		inline constexpr int16_t operator[](size_t Index) const
		{
			if (Index < 0 || Index >= GetLength())
				return 0xCDCD;

			return PatternBytes[Index];
		}
	};

	/* Pseudo-implementation of the boyer-moore algorithm */
	template<int32_t PatternLengthBytes, int32_t PatternLengthWithoutWildcards>
	inline const void* FindPattern(const uint8_t* Memory, int32_t SearchRange, const PatternInfo<PatternLengthBytes, PatternLengthWithoutWildcards>& Pattern)
	{
		if (SearchRange <= 0x0)
			return nullptr;

		const int32_t PatternMaxIndex = Pattern.GetLength() - 1;

		for (int i = Pattern.GetLength(); i < SearchRange; i++)
		{
			for (int j = 0x0; j < Pattern.GetLength(); j++)
			{
				const int32_t CurrentPatternIndex = PatternMaxIndex - j;

				const int16_t CurrentPatternValue = Pattern[CurrentPatternIndex];
				const uint8_t CurrentInnerLoopValue = Memory[i - j];

				/* Wildcard, skip this byte */
				if (CurrentPatternValue == -1)
					continue;

				if (static_cast<uint8_t>(CurrentPatternValue) == CurrentInnerLoopValue)
				{
					if (j == PatternMaxIndex)
						return Memory + i - j;

					continue;
				}

				const ByteInfo Info = Pattern.GetByteInfo(CurrentInnerLoopValue);

				if (!Info.IsInPattern())
				{
					i += Pattern.GetLength();
					break;
				}

				i += Info.GetByteSkipCount();
				break;
			}
		}
	}
}