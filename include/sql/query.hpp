#pragma once

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

#include "cexpr/string.hpp"

#include "ra/cross.hpp"
#include "ra/join.hpp"
#include "ra/natural.hpp"
#include "ra/projection.hpp"
#include "ra/relation.hpp"
#include "ra/rename.hpp"
#include "ra/selection.hpp"

#include "sql/column.hpp"
#include "sql/tokens.hpp"
#include "sql/predicate.hpp"
#include "sql/row.hpp"

namespace sql
{

	// anonymous namespace to hold helper data structures and functions
	namespace
	{

		template <std::size_t Pos, typename Node>
		struct context
		{
			using node = Node;
			static constexpr std::size_t pos = Pos;
		};

		template <typename Type, std::size_t Name, std::size_t Next>
		struct colinfo
		{
			using type = Type;
			static constexpr std::size_t name = Name;
			static constexpr std::size_t next = Next;
		};

		template <cexpr::string Name, typename Row>
		constexpr bool exists() noexcept
		{
			if constexpr (std::is_same_v<Row, sql::void_row>)
			{
				return false;
			}
			else
			{
				if constexpr (Row::column::name == Name)
				{
					return true;
				}
				else
				{
					return exists<Name, typename Row::next>();
				}
			}
		}

		template <typename Type, typename Char, std::size_t N>
		constexpr value<Type> convert(cexpr::string<Char, N> const& str)
		{
			auto curr{ str.cbegin() }, end{ str.cend() };
			constexpr Char dot{ '.' }, zro{ '0' }, min{ '-' };
			Type acc{}, sign{ 1 }, scalar{ 10 };

			if (*curr == min)
			{
				sign = -1;
				++curr;
			}

			while (curr != end && *curr != dot)
			{
				acc = (acc * scalar) + (*curr - zro);
				++curr;
			}

			if (curr != end && *curr == dot)
			{
				scalar = 1;
				++curr;

				while(curr != end)
				{
					acc += (*curr - zro) * (scalar /= Type{ 10 });
					++curr;
				}
			}

			return value<Type>{ acc * sign };
		}

		inline constexpr bool isquote(std::string_view const& tv) noexcept
		{
			return tv == "\"" || tv == "'";
		}

		inline constexpr bool isor(std::string_view const& tv) noexcept
		{
			return tv == "OR" || tv == "or";
		}

		inline constexpr bool isand(std::string_view const& tv) noexcept
		{
			return tv == "AND" || tv == "and";
		}

		inline constexpr bool isnot(std::string_view const& tv) noexcept
		{
			return tv == "NOT" || tv == "not";
		}

		inline constexpr bool isnatural(std::string_view const& tv) noexcept
		{
			return tv == "NATURAL" || tv == "natural";
		}

		inline constexpr bool isjoin(std::string_view const& tv) noexcept
		{
			return tv == "JOIN" || tv == "join";
		}

		inline constexpr bool iswhere(std::string_view const& tv) noexcept
		{
			return tv == "WHERE" || tv == "where";
		}

		inline constexpr bool isfrom(std::string_view const& tv) noexcept
		{
			return tv == "FROM" || tv == "from";
		}

		inline constexpr bool isas(std::string_view const& tv) noexcept
		{
			return tv == "AS" || tv == "as";
		}

		inline constexpr bool isselect(std::string_view const& tv) noexcept
		{
			return tv == "SELECT" || tv == "select";
		}

		inline constexpr bool iscomma(std::string_view const& tv) noexcept
		{
			return tv == ",";
		}

		constexpr bool isintegral(std::string_view const& tv) noexcept
		{
			bool result{ false };

			for (auto c : tv)
			{
				result |= (c == '.');
			}

			return !result;
		}

		constexpr bool isdigit(char c) noexcept
		{
			return (c >= '0' && c <= '9') || c == '-' || c == '.';
		}

		constexpr bool iscomp(std::string_view const& tv) noexcept
		{
			return tv[0] == '=' || tv[0] == '!' || tv[0] == '<' || tv[0] == '>';
		}

		constexpr bool iscolumn(std::string_view const& tv) noexcept
		{
			return !iscomma(tv) && !isas(tv) && !isfrom(tv);
		}

		constexpr bool isseparator(std::string_view const& tv) noexcept
		{
			return iscomma(tv) || isfrom(tv);
		}

	} // namespace

	// structured binding compliant iterator for query objects
	template <typename Expr>
	class query_iterator
	{
	public:
		using row_type = std::remove_cvref_t<typename Expr::output_type>;

		// seeds row datamember for first dereference
		query_iterator(bool end) : end_{ end }, row_{}
		{
			operator++();
		}

		bool operator==(query_iterator const& it) const
		{
			return end_ == it.end_;
		}

		bool operator!=(query_iterator const& it) const
		{
			return !(*this == it);
		}

		row_type const& operator*() const
		{
			return row_;
		}

		query_iterator& operator++()
		{
			if (!end_)
			{
				try
				{
					row_ = Expr::next();
				}
				catch (ra::data_end const& e)
				{
					end_ = true;
				}
			}

			return *this;
		}

	private:
		bool end_{};
		row_type row_{};
	};

	template <cexpr::string Str, typename... Schemas>
	class query
	{
	private:
		// where predicate terminal parsing 
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_terms() noexcept
		{
			if constexpr (tokens_[Pos] == "(")
			{
				constexpr auto next{ parse_or<Pos + 1, Row>() };

				using node = typename decltype(next)::node;

				static_assert(tokens_[next.pos] == ")", "No closing paranthesis found.");

				return context<next.pos + 1, node>{};
			}
			else if constexpr (isquote(tokens_[Pos]))
			{
				constexpr cexpr::string<char, tokens_[Pos + 1].length() + 1> name{ tokens_[Pos + 1] };

				using str = decltype(name);
				using node = sql::constant<value<str>{ name }, Row>;

				static_assert(isquote(tokens_[Pos + 2]), "No closing quote found.");

				return context<Pos + 3, node>{};
			}
			else if constexpr (isdigit(tokens_[Pos][0]))
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };
				
				using val = decltype(isintegral(tokens_[Pos]) ? std::int64_t{} : double{});
				using node = sql::constant<sql::convert<val>(name), Row>;

				return context<Pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = sql::variable<name, Row>;

				return context<Pos + 1, node>{};
			}
		}

		// parses a single compare operation
		template <typename Left, typename Row>
		static constexpr auto recurse_comparison() noexcept
		{
			if constexpr (!iscomp(tokens_[Left::pos]))
			{
				return Left{};
			}
			else
			{
				constexpr auto next{ parse_terms<Left::pos + 1, Row>() };
				constexpr cexpr::string<char, tokens_[Left::pos].length() + 1> name{ tokens_[Left::pos] };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<name, Row, typename Left::node, ranode>;

				return context<next.pos, node>{};
			}			
		}

		// descend further and attempt to parse a compare operation
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_comparison() noexcept
		{
			using left = decltype(parse_terms<Pos, Row>());
			
			return recurse_comparison<left, Row>();
		}

		// attempt to parse a negation operation then descend further
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_negation() noexcept
		{
			if constexpr (isnot(tokens_[Pos]))
			{
				constexpr auto next{ parse_comparison<Pos + 1, Row>() };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<"NOT", Row, ranode>;

				return context<next.pos, node>{};
			}
			else
			{
				return parse_comparison<Pos, Row>();
			}
		}

		// recursively parse chained AND operations
		template <typename Left, typename Row>
		static constexpr auto recurse_and() noexcept
		{
			if constexpr (!isand(tokens_[Left::pos]))
			{
				return Left{};
			}
			else
			{
				constexpr auto next{ parse_negation<Left::pos + 1, Row>() };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<"AND", Row, typename Left::node, ranode>;

				return recurse_and<context<next.pos, node>, Row>();
			}
		}

		// descend further then attempt to parse AND operations
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_and() noexcept
		{
			using left = decltype(parse_negation<Pos, Row>());
			
			return recurse_and<left, Row>();
		}
		
		// recursively parse chained OR operations
		template <typename Left, typename Row>
		static constexpr auto recurse_or() noexcept
		{
			if constexpr (!isor(tokens_[Left::pos]))
			{
				return Left{};
			}
			else
			{
				constexpr auto next{ parse_and<Left::pos + 1, Row>() };
				
				using ranode = typename decltype(next)::node;
				using node = sql::operation<"OR", Row, typename Left::node, ranode>;

				return recurse_or<context<next.pos, node>, Row>();
			}
		}

		// descend further then attempt to parse OR operations
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_or() noexcept
		{
			using left = decltype(parse_and<Pos, Row>());
			
			return recurse_or<left, Row>();
		}

		// find correct schema for terminal relation
		template <cexpr::string Name, std::size_t Id, typename Schema, typename... Others>
		static constexpr auto recurse_schemas()
		{
			if constexpr (Name == Schema::name)
			{
				return ra::relation<Schema, Id>{};
			}
			else
			{
				static_assert(sizeof...(Others) != 0, "Schema name used in JOIN was not provided.");

				return recurse_schemas<Name, Id, Others...>();
			}
		}

		// wrapper function to determine terminal relation
		template <std::size_t Pos>
		static constexpr auto parse_schema()
		{
			if constexpr (tokens_[Pos] == "(")
			{
				constexpr auto next{ parse_root<Pos + 1>() };
				
				using node = typename decltype(next)::node;

				static_assert(tokens_[next.pos] == ")", "No closing paranthesis found.");

				return context<next.pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = decltype(recurse_schemas<name, Pos, Schemas...>());

				return context<Pos + 1, node>{};
			}
		}

		// stub which will choose the specific join RA node
		template <std::size_t Pos, typename Left, typename Right>
		static constexpr auto choose_join()
		{
			if constexpr (isnatural(tokens_[Pos]))
			{
				return ra::natural<Left, Right>{};
			}
			else
			{
				return ra::cross<Left, Right>{};	
			}
		}

		// parses join colinfo if a join is present else returns the single relation terminal
		template <std::size_t Pos>
		static constexpr auto parse_join() noexcept
		{
			constexpr auto lnext{ parse_schema<Pos>() };

			using lnode = typename decltype(lnext)::node;

			if constexpr (lnext.pos + 2 < tokens_.count() && isjoin(tokens_[lnext.pos + 1]))
			{
				constexpr auto rnext{ parse_schema<lnext.pos + 2>() };

				using rnode = typename decltype(rnext)::node;
				using join = decltype(choose_join<lnext.pos, lnode, rnode>());

				return context<rnext.pos, join>{};
			}
			else
			{
				return context<lnext.pos, lnode>{};
			}
		}

		// starting point to parse everything after the from keyword
		template <std::size_t Pos>
		static constexpr auto parse_from() noexcept
		{
			static_assert(isfrom(tokens_[Pos]), "Expected 'FROM' token not found.");

			constexpr auto next{ parse_join<Pos + 1>() };

			using node = typename decltype(next)::node;

			if constexpr (next.pos < tokens_.count() && iswhere(tokens_[next.pos]))
			{
				using output = std::remove_cvref_t<typename node::output_type>;

				constexpr auto predicate{ parse_or<next.pos + 1, output>() };

				using pnext = typename decltype(predicate)::node;
				using snode = ra::selection<pnext, node>;

				return context<predicate.pos, snode>{};
			}
			else
			{
				return context<next.pos, node>{};	
			}
		}

		// recursively searches all schemas for the a matching column
		template <cexpr::string Name, typename Schema, typename... Others>
		static constexpr auto recurse_types()
		{
			if constexpr (sql::exists<Name, typename Schema::row_type>())
			{
				return decltype(sql::get<Name>(typename Schema::row_type{})){};
			}
			else
			{
				static_assert(sizeof...(Others) != 0, "Column name was not present in any schema.");

				return recurse_types<Name, Others...>();
			}
		}

		// wrapper to determine the type for the the column
		template <std::size_t Pos>
		static constexpr auto column_type() noexcept
		{
			constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

			return recurse_types<name, Schemas...>();
		}

		// asserts token is column separator, and if comma returns one past the comma else start position
		template <std::size_t Pos>
		static constexpr std::size_t next_column() noexcept
		{
			static_assert(isseparator(tokens_[Pos]), "Expected ',' or 'FROM' token following column.");

			if constexpr (iscomma(tokens_[Pos]))
			{
				return Pos + 1;
			}
			else
			{
				return Pos;
			}
		}

		template <std::size_t Pos, bool Rename>
		static constexpr auto parse_colinfo() noexcept
		{
			static_assert(iscolumn(tokens_[Pos]), "Invalid token starting column delcaration.");

			constexpr bool rename{ isas(tokens_[Pos + 1]) && iscolumn(tokens_[Pos + 2]) };

			using col = decltype(column_type<Pos>());

			if constexpr (Rename && rename)
			{
				constexpr auto next{ next_column<Pos + 3>() };

				return colinfo<col, Pos + 2, next>{};
			}
			else if constexpr (rename)
			{
				constexpr auto next{ next_column<Pos + 3>() };

				return colinfo<col, Pos, next>{};
			}
			else
			{
				constexpr auto next{ next_column<Pos + 1>() };

				return colinfo<col, Pos, next>{};
			}
		}

		// recursively parse all columns projected/renamed in the query
		template <std::size_t Pos, bool Rename>
		static constexpr auto recurse_columns() noexcept
		{
			if constexpr (isfrom(tokens_[Pos]))
			{
				return context<Pos, sql::void_row>{};
			}
			else
			{
				constexpr auto info{ parse_colinfo<Pos, Rename>() };
				constexpr cexpr::string<char, tokens_[info.name].length() + 1> name{ tokens_[info.name] };
				constexpr auto child{ recurse_columns<info.next, Rename>() };

				using next = std::remove_const_t<typename decltype(child)::node>;
				using col = std::remove_const_t<decltype(sql::column<name, typename decltype(info)::type>{})>;
				using node = sql::row<col, next>;

				return context<child.pos, node>{};
			}
		}

		// wrapper to parse columns as a projection RA node
		template <std::size_t Pos>
		static constexpr auto parse_projection() noexcept
		{
			constexpr auto proj{ recurse_columns<Pos, false>() };
			constexpr auto next{ parse_from<proj.pos>() };

			using ranode = typename decltype(proj)::node;
			using node = ra::projection<ranode, typename decltype(next)::node>;

			return context<next.pos, node>{};
		}

		// wrapper to parse columns as a rename RA node
		template <std::size_t Pos>
		static constexpr auto parse_rename() noexcept
		{
			constexpr auto next = parse_projection<Pos>();

			using ranode = typename decltype(recurse_columns<Pos, true>())::node;
			using node = ra::rename<ranode, typename decltype(next)::node>;

			return context<next.pos, node>{};
		}

		// attempts to match column rename operation pattern on a column
		template <std::size_t Pos>
		static constexpr bool has_rename() noexcept
		{
			if constexpr (isfrom(tokens_[Pos]) || isfrom(tokens_[Pos + 2]))
			{
				return false;
			}
			else if constexpr (iscolumn(tokens_[Pos]) && isas(tokens_[Pos + 1]) && iscolumn(tokens_[Pos + 2]))
			{
				return true;
			}
			else
			{
				constexpr bool comma{ iscomma(tokens_[Pos + 1]) };

				static_assert(comma || isfrom(tokens_[Pos + 1]), "Expected ',' or 'FROM' token following column.");

				if constexpr (comma)
				{
					return has_rename<Pos + 2>();
				}
				else
				{
					return has_rename<Pos + 1>();
				}
			}
		}

		// decide RA node to root the expression tree
		template <std::size_t Pos>
		static constexpr auto parse_root() noexcept
		{
			static_assert(isselect(tokens_[Pos]), "Expected 'SELECT' token not found.");

			if constexpr (tokens_[Pos + 1] == "*")
			{
				return parse_from<Pos + 2>();
			}
			else if constexpr (has_rename<Pos + 1>())
			{
				return parse_rename<Pos + 1>();
			}
			else
			{
				return parse_projection<Pos + 1>();
			}
		}

		static constexpr sql::tokens<char, sql::preprocess(Str)> tokens_{ Str };

		using expression = typename decltype(parse_root<0>())::node;

		bool empty_;
	
	public:
		using iterator = query_iterator<expression>;
		using row_type = expression::output_type;

		query(Schemas const&... tables)
		{
			try 
			{
				expression::seed(tables...);
				empty_ = false;
			}
			catch(ra::data_end const& e)
			{
				empty_ = true;
			}
		}

		~query()
		{
			expression::reset();
		}

		iterator begin() const
		{
			return iterator{ empty_ };
		}

		iterator end() const
		{
			return iterator{ true };
		}
	};

} // namespace sql
