#pragma once

#include "ra/operation.hpp"

#include "sql/row.hpp"

namespace ra
{

	template <typename Output, typename Input>
	class rename : public ra::unary<Input>
	{
		using input_type = typename ra::unary<Input>::input_type;
	public:
		using output_type = Output;

		static auto& next()
		{
			fold<output_type, input_type>(output_row, Input::next());
			
			return output_row;
		}

	private:
		template <typename Dest, typename Src>
		static inline constexpr void fold(Dest& dest, Src const& src)
		{
			if constexpr (Dest::depth == 0)
			{
				return;
			}
			else
			{
				dest.head() = src.head();
				fold<typename Dest::next, typename Src::next>(dest.tail(), src.tail());
			}
		}

		static output_type output_row;
	};

	template <typename Output, typename Input>
	typename rename<Output, Input>::output_type rename<Output, Input>::output_row{};
	
} // namespace ra
