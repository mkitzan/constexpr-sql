#pragma once

#include "ra/join.hpp"
#include "ra/relation.hpp"

namespace ra
{

	template <typename LeftInput, typename RightInput>
	class cross : public ra::join<LeftInput, RightInput>
	{
		using join_type = ra::join<LeftInput, RightInput>;
	public:
		using output_type = join_type::output_type;

		static auto&& next()
		{
			try
			{
				copy(join_type::output_row, RightInput::next());
			}
			catch(ra::data_end const& e)
			{
				copy(join_type::output_row, LeftInput::next());
				RightInput::reset();
				copy(join_type::output_row, RightInput::next());
			}

			return std::move(join_type::output_row);
		}
	};

} // namespace ra
