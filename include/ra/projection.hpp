#pragma once

#include "ra/operation.hpp"

#include "sql/row.hpp"

namespace ra
{

	template <typename Output, typename Input>
	class projection : public ra::unary<Input>
	{
		using input_type =  typename ra::unary<Input>::input_type;
	public:
		using output_type = Output;

		static auto& next()
		{
			fold<output_type>(output_row, Input::next());
			
			return output_row;
		}

	private:
		template <typename Dest>
		static inline constexpr void fold(Dest& dest, input_type const& src)
		{
			if constexpr (Dest::depth == 0)
			{
				return;
			}
			else
			{
				dest.head() = sql::get<Dest::column::name>(src);
				fold<typename Dest::next>(dest.tail(), src);	
			}
		}

		static output_type output_row;
	};

	template <typename Output, typename Input>
	typename projection<Output, Input>::output_type projection<Output, Input>::output_row{};

} // namespace ra
