#pragma once

#include "cexpr/string.hpp"

namespace sql
{

	template <cexpr::string Name, typename Type>
	struct column
	{
		static constexpr auto name{ Name };
		using type = Type;
	};

} // namespace sql
