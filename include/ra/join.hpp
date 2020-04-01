#pragma once

#include <type_traits>
#include <unordered_map>
#include <vector>

#include "ra/operation.hpp"
#include "ra/relation.hpp"

#include "sql/row.hpp"

namespace ra
{

	namespace
	{

		template <typename Left, typename Right>
		constexpr auto recr_merge()
		{
			if constexpr (std::is_same_v<Left, sql::void_row>)
			{
				return Right{};
			}
			else
			{
				return sql::row<typename Left::column, decltype(recr_merge<typename Left::next, Right>())>{};
			}
		}

		template <typename Left, typename Right>
		inline constexpr auto merge()
		{
			if constexpr (Left::column::name == Right::column::name)
			{
				return recr_merge<Left, typename Right::next>();
			}
			else
			{
				return recr_merge<Left, Right>();
			}
		}

		template <typename Dest, typename Row>
		constexpr void recr_copy(Dest& dest, Row const& src)
		{
			if constexpr (std::is_same_v<Row, sql::void_row>)
			{
				return;
			}
			else
			{
				dest.head() = src.head();
				recr_copy(dest.tail(), src.tail());
			}
		}

		template <typename Dest, typename Row>
		inline constexpr void copy(Dest& dest, Row const& src)
		{
			if constexpr (Dest::column::name == Row::column::name)
			{
				recr_copy(dest, src);
			}
			else
			{
				copy(dest.tail(), src);
			}
		}

	} // namespace

	template <typename LeftInput, typename RightInput>
	class join : public ra::binary<LeftInput, RightInput>
	{
		using binary_type = ra::binary<LeftInput, RightInput>;
		using left_type = typename binary_type::left_type;
		using right_type = typename binary_type::right_type;
	public:
		using output_type = decltype(merge<left_type, right_type>());

		template <typename... Inputs>
		static inline void seed(Inputs const&... rs)
		{
			binary_type::seed(rs...);
			copy(output_row, LeftInput::next());
		}

		static inline void reset()
		{
			binary_type::reset();
			copy(output_row, LeftInput::next());
		}

		static output_type output_row;
	};

	template <typename LeftInput, typename RightInput>
	typename join<LeftInput, RightInput>::output_type join<LeftInput, RightInput>::output_row{};

	template <typename LeftInput, typename RightInput>
	class cross : public ra::join<LeftInput, RightInput>
	{
		using join_type = ra::join<LeftInput, RightInput>;
	public:
		using output_type = join_type::output_type;

		static auto& next()
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

			return join_type::output_row;
		}
	};

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

		static auto& next()
		{
			while (curr == end)
			{
				copy(join_type::output_row, LeftInput::next());
				auto const& active{ row_cache[join_type::output_row.head()] };
				curr = active.cbegin();
				end = active.cend();
			}

			copy(join_type::output_row, *curr++);
			
			return join_type::output_row;
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
