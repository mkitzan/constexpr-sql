#pragma once

#include <type_traits>
#include <vector>
#include <unordered_map>

#include "ra/join.hpp"
#include "ra/relation.hpp"

namespace ra
{

	template <typename LeftInput, typename RightInput>
	class natural : public ra::join<LeftInput, RightInput>
	{
		using join_type = ra::join<LeftInput, RightInput>;
		using key_type = std::remove_cvref_t<decltype(LeftInput::next().head())>;
		using value_type = std::vector<std::remove_cvref_t<decltype(RightInput::next().tail())>>;
		using map_type = std::unordered_map<key_type, value_type>;
	public:
		using output_type = join_type::output_type;

		template <typename... Inputs>
		static void seed(Inputs const&... rs)
		{
			join_type::seed(rs...);
			
			if (row_cache.empty())
			{
				try
				{
					for (;;)
					{
						auto const& row{ RightInput::next() };
						row_cache[row.head()].push_back(row.tail());
					}
				}
				catch(ra::data_end const& e)
				{
					RightInput::reset();
				}
			}

			auto const& active{ row_cache[join_type::output_row.head()] };
			curr = active.cbegin();
			end = active.cend();
		}

		static auto&& next()
		{
			while (curr == end)
			{
				copy(join_type::output_row, LeftInput::next());
				auto const& active{ row_cache[join_type::output_row.head()] };
				curr = active.cbegin();
				end = active.cend();
			}

			copy(join_type::output_row, *curr++);
			
			return std::move(join_type::output_row);
		}

	private:
		static map_type row_cache;
		static value_type::const_iterator curr;
		static value_type::const_iterator end;
	};

	template <typename LeftInput, typename RightInput>
	typename natural<LeftInput, RightInput>::map_type natural<LeftInput, RightInput>::row_cache{};

	template <typename LeftInput, typename RightInput>
	typename natural<LeftInput, RightInput>::value_type::const_iterator natural<LeftInput, RightInput>::curr;

	template <typename LeftInput, typename RightInput>
	typename natural<LeftInput, RightInput>::value_type::const_iterator natural<LeftInput, RightInput>::end;

} // namespace ra
