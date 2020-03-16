#pragma once

#include <cstdint>

#include <type_traits>

namespace KTechLab::Types {
	enum class Result : bool {
		Success = true,
		Failure = false
	};

	static constexpr inline bool operator ! (Result result) { return !bool(result); }

	namespace detail {
		// Conditionally makes an integer-type signed or unsigned
		template <typename T, bool MakeSigned> using ConditionalSigned =
			std::conditional_t<
				MakeSigned,
				std::make_signed_t<T>,
				std::make_unsigned_t<T>
			>;

		// Returns the size of a type in bits
		template <typename T> static constexpr const size_t TypeBits = sizeof(T) * CHAR_BIT;

		// Chooses between a signed/unsigned T or NT depending on whether T fits the requested bits.
		template <size_t Bits, bool IsSigned, typename T, typename NT> using SizeConditional =
			std::conditional_t<
				TypeBits<T> >= Bits,
				ConditionalSigned<T, IsSigned>,
				NT
			>;

		// Automatically gets a minimum-sized integer that fits the provided bit size
		template <size_t Bits, bool IsSigned> using BitSizedInt =
			SizeConditional<Bits, IsSigned, char,
				SizeConditional<Bits, IsSigned, short,
					SizeConditional<Bits, IsSigned, int,
						SizeConditional<Bits, IsSigned, long,
							SizeConditional<Bits, IsSigned, long long,
								// GCC and friends support 128-bit integers as extensions
								// TODO : It can theoretically be added via a header with constexpr
								// for other toolchains, and would be very useful in this project.
#if defined(__SIZEOF_INT128__)
								SizeConditional<Bits, IsSigned, __int128,
									void
								>
#else
								void
#endif
							>
						>
					>
				>
			>;
	}

	// Avoid using 'long' in the code, prefer llong/ullong, as some platforms such as
	// Windows use an integer-sized long.

	using llong = long long;

	using uchar = unsigned char;
	using ushort = unsigned short;
	using uint = unsigned int;
	using ullong = unsigned long long;

#define KTL_DEFINE_SIGNED_INT(bits) \
	using int ## bits = detail::BitSizedInt<bits, true>
	KTL_DEFINE_SIGNED_INT(8);
	KTL_DEFINE_SIGNED_INT(16);
	KTL_DEFINE_SIGNED_INT(32);
	KTL_DEFINE_SIGNED_INT(64);
	KTL_DEFINE_SIGNED_INT(128);
#undef KTL_DEFINE_SIGNED_INT

#define KTL_DEFINE_UNSIGNED_INT(bits) \
	using uint ## bits = detail::BitSizedInt<bits, false>
	KTL_DEFINE_UNSIGNED_INT(8);
	KTL_DEFINE_UNSIGNED_INT(16);
	KTL_DEFINE_UNSIGNED_INT(32);
	KTL_DEFINE_UNSIGNED_INT(64);
	KTL_DEFINE_UNSIGNED_INT(128);
#undef KTL_DEFINE_UNSIGNED_INT

	using intsz = std::make_signed_t<decltype(sizeof(0))>;
	using uintsz = std::make_unsigned_t<decltype(sizeof(0))>;

	using intptr = std::intptr_t;
	using uintptr = std::uintptr_t;

	using real = double;

	template <typename T>
	using carray = T[];
	using cstring = carray<char>;

	// TODO : Move me to type traits or something
	template <typename T>
	static constexpr const T MaxValue = std::numeric_limits<T>::max();

	template <typename T>
	static constexpr const T MinValue = std::numeric_limits<T>::lowest();

	template <typename T>
	static constexpr const T SmallestValue = std::numeric_limits<T>::min();
}
