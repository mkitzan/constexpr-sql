#pragma once

#include "ra/operation.hpp"

namespace ra
{

	template <typename Predicate, typename Input>
	class selection : public ra::unary<Input>
	{
		using input_type = typename ra::unary<Input>::input_type;
	public:
		using output_type = input_type;		

		static auto& next()
		{
			output_row = Input::next();

			while(!Predicate::eval(output_row))
			{
				output_row = Input::next();
			}

			return output_row;
		}

	private:
		static output_type output_row;
	};

	template <typename Output, typename Input>
	typename selection<Output, Input>::output_type selection<Output, Input>::output_row{};

} // namespace ra
