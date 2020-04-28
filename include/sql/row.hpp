#pragma once

#include <type_traits>
#include <utility>
#include <vector>

#include "cexpr/string.hpp"

namespace sql
{

	struct void_row
	{
		static constexpr std::size_t depth{ 0 };
	};

	template <typename Col, typename Next>
	class row
	{
	public:
		using column = Col;
		using next = Next;
		static constexpr std::size_t depth{ 1 + next::depth };

		row() = default;

		template <typename... ColTs>
		row(column::type const& val, ColTs const&... vals) : value_{ val }, next_{ vals... }
		{}

		template <typename... ColTs>
		row(column::type&& val, ColTs&&... vals) : value_{ std::forward<column::type>(val) }, next_{ std::forward<ColTs>(vals)... }
		{}

		inline constexpr next const& tail() const noexcept
		{
			return next_;
		}

		inline constexpr next& tail() noexcept
		{
			return next_;
		}

		inline constexpr column::type const& head() const noexcept
		{
			return value_;
		}

		inline constexpr column::type& head() noexcept
		{
			return value_;
		}
	
	private:
		column::type value_;
		next next_;
	};

	template <typename Col, typename... Cols>
	struct variadic_row
	{
	private:
		static inline constexpr auto resolve() noexcept
		{
			if constexpr (sizeof...(Cols) != 0)
			{
				return typename variadic_row<Cols...>::row_type{};
			}
			else
			{
				return void_row{};
			}
		}

	public:
		using row_type = row<Col, decltype(resolve())>;
	};

	// user function to query row elements by column name
	template <cexpr::string Name, typename Row>
	constexpr auto const& get(Row const& r) noexcept
	{
		static_assert(!std::is_same_v<Row, sql::void_row>, "Name does not match a column name.");

		if constexpr (Row::column::name == Name)
		{
			return r.head();
		}
		else
		{
			return get<Name>(r.tail());
		}
	}

	// compiler function used by structured binding declaration
	template <std::size_t Pos, typename Row>
	constexpr auto const& get(Row const& r) noexcept
	{
		static_assert(Pos < Row::depth, "Position is larger than number of row columns.");

		if constexpr (Pos == 0)
		{
			return r.head();
		}
		else
		{
			return get<Pos - 1>(r.tail());
		}
	}

	// function to assign a value to a column's value in a row
	template <cexpr::string Name, typename Row, typename Type>
	constexpr void set(Row& r, Type const& value)
	{
		static_assert(!std::is_same_v<Row, sql::void_row>, "Name does not match a column name.");

		if constexpr (Row::column::name == Name)
		{
			r.head() = value;
		}
		else
		{
			set<Name>(r.tail(), value);
		}
	}

} // namespace sql

// STL injections to allow row to be used in structured binding declarations
namespace std
{

	template <typename Col, typename Next>
	class tuple_size<sql::row<Col, Next>> : public integral_constant<size_t, sql::row<Col, Next>::depth>
	{};

	template <size_t Index, typename Col, typename Next>
	struct tuple_element<Index, sql::row<Col, Next>>
	{
		using type = decltype(sql::get<Index>(sql::row<Col, Next>{}));
	};

} // namespace std
