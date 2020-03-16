#pragma once

#if __GNUC__
#	define _callback(name, ...) __attribute__((callback(name, __VA_ARGS__)))
#	define _diagnose(condition, message) __attribute__((diagnose_if((condition), message)))
#	define _non_unique [[no_unique_address]]
#	define _forceinline __attribute__((always_inline))
#	if __clang__
#		define _trivial [[clang::trivial_abi]]
#		define _fallthrough [[fallthrough]]
#	else
#		define _fallthrough __attribute__((fallthrough))
#		define _nullable
#		define _nonnull
#		define _trivial
#	endif
#else
#	if _MSC_VER
#		define _forceinline __forceinline
#	else
#		define _forceinline
#	endif
#	define _callback(name, ...)
#	define _diagnose(condition, message)
#	define _nullable
#	define _nonnull
#endif

#define restrict __restrict
