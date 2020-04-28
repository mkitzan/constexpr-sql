#pragma once

#include <cstddef>

#include "cexpr/string.hpp"

namespace sql
{

	namespace
	{

		// shim to allow all value types like double or float to be used as non-type template parameters.
		template <typename Type>
		struct value
		{
			constexpr value(Type v) : val{ v }
			{}

			Type val;
		};

	} // namespace

	template <cexpr::string Op, typename Row, typename Left, typename Right=void>
	struct operation
	{
		static constexpr bool eval(Row const& row) noexcept
		{
			if constexpr (Op == "=")
			{
				return Left::eval(row) == Right::eval(row);
			}
			else if constexpr (Op == ">")
			{
				return Left::eval(row) > Right::eval(row);
			}
			else if constexpr(Op == "<")
			{
				return Left::eval(row) < Right::eval(row);
			}
			else if constexpr(Op == ">=")
			{
				return Left::eval(row) >= Right::eval(row);
			}
			else if constexpr(Op == "<=")
			{
				return Left::eval(row) <= Right::eval(row);
			}
			else if constexpr(Op == "!=" || Op == "<>")
			{
				return Left::eval(row) != Right::eval(row);
			}
			else if constexpr(Op == "AND")
			{
				return Left::eval(row) && Right::eval(row);
			}
			else if constexpr(Op == "OR")
			{
				return Left::eval(row) || Right::eval(row);
			}
			else if constexpr(Op == "NOT")
			{
				return !Left::eval(row);
			}
		}
	};

	template <cexpr::string Column, typename Row>
	struct variable
	{
		static constexpr auto eval(Row const& row) noexcept
		{
			return sql::get<Column>(row);
		}
	};

	template <auto Const, typename Row>
	struct constant
	{
		static constexpr auto eval([[maybe_unused]] Row const& row) noexcept
		{
			return Const.val;
		}
	};

} // namespace sql
