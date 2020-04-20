#pragma once

#include <array>
#include <string>
#include <string_view>
#include <type_traits>

#include "cexpr/string.hpp"

#include "ra/join.hpp"
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
		struct TreeNode
		{
			using node = Node;
			static constexpr std::size_t pos = Pos;
		};

		template <typename Type, std::size_t Name, std::size_t Next>
		struct ColInfo
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

		template <typename ValT, typename CharT, std::size_t N>
		constexpr value<ValT> convert(cexpr::string<CharT, N> const& str)
		{
			auto curr{ str.cbegin() }, end{ str.cend() };
			constexpr CharT nul{ '\0' }, dot{ '.' }, zro{ '0' }, min{ '-' };
			ValT acc{}, sign{ 1 }, scalar{ 10 };

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
					acc += (*curr - zro) * (scalar /= ValT{ 10 });
					++curr;
				}
			}

			return value<ValT>{ acc * sign };
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
			return c == '-' || c == '.' || (c >= '0' && c <= '9');
		}

		constexpr bool iscomp(std::string_view const& tv) noexcept
		{
			return tv[0] == '=' || tv[0] == '!' || tv[0] == '<' || tv[0] == '>';
		}

		constexpr bool isor(std::string_view const& tv) noexcept
		{
			return tv == "or" || tv == "OR";
		}

		constexpr bool isand(std::string_view const& tv) noexcept
		{
			return tv == "and" || tv == "AND";
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

				return TreeNode<next.pos + 1, node>{};
			}
			else if constexpr (tokens_[Pos] == "\'" || tokens_[Pos] == "\"")
			{
				constexpr cexpr::string<char, tokens_[Pos + 1].length() + 1> name{ tokens_[Pos + 1] };

				using str = decltype(name);
				using node = sql::constant<value<str>{ name }, Row>;

				return TreeNode<Pos + 3, node>{};
			}
			else if constexpr (isdigit(tokens_[Pos][0]))
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };
				
				using val = decltype(isintegral(tokens_[Pos]) ? std::int64_t{} : double{});
				using node = sql::constant<sql::convert<val>(name), Row>;

				return TreeNode<Pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = sql::variable<name, Row>;

				return TreeNode<Pos + 1, node>{};
			}
		}

		// parses a single compare operation
		template <typename Left, typename Row>
		static constexpr auto recurse_comp() noexcept
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

				return TreeNode<next.pos, node>{};
			}			
		}

		// descend further and attempt to parse a compare operation
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_comp() noexcept
		{
			using left = decltype(parse_terms<Pos, Row>());
			
			return recurse_comp<left, Row>();
		}

		// attempt to parse a negation operation then descend further
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_negation() noexcept
		{
			if constexpr (tokens_[Pos] == "not" || tokens_[Pos] == "NOT")
			{
				constexpr auto next{ parse_comp<Pos + 1, Row>() };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<"NOT", Row, ranode>;

				return TreeNode<next.pos, node>{};
			}
			else
			{
				return parse_comp<Pos, Row>();
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

				return recurse_and<TreeNode<next.pos, node>, Row>();
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

				return recurse_or<TreeNode<next.pos, node>, Row>();
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
				return recurse_schemas<Name, Id, Others...>();
			}
		}

		// wrapper function to determine terminal relation
		template <std::size_t Pos>
		static constexpr auto parse_schema()
		{
			if constexpr (tokens_[Pos] == "(")
			{
				constexpr auto next{ parse_root<Pos + 2>() };
				
				using node = typename decltype(next)::node;

				return TreeNode<next.pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = decltype(recurse_schemas<name, Pos, Schemas...>());

				return TreeNode<Pos + 1, node>{};
			}
		}

		// stub which will choose the specific join RA node
		template <std::size_t Pos, typename Left, typename Right>
		static constexpr auto choose_join()
		{
			if constexpr (tokens_[Pos] == "natural" || tokens_[Pos] == "NATURAL")
			{
				return ra::natural<Left, Right>{};
			}
			else
			{
				return ra::cross<Left, Right>{};	
			}
		}

		// parses join info if a join is present else returns the single relation terminal
		template <std::size_t Pos>
		static constexpr auto parse_join() noexcept
		{
			constexpr auto lnext{ parse_schema<Pos>() };

			using lnode = typename decltype(lnext)::node;

			if constexpr (lnext.pos + 2 < tokens_.count() && (tokens_[lnext.pos + 1] == "join" || tokens_[lnext.pos + 1] == "JOIN"))
			{
				constexpr auto rnext{ parse_schema<lnext.pos + 2>() };

				using rnode = typename decltype(rnext)::node;
				using join = decltype(choose_join<lnext.pos, lnode, rnode>());

				return TreeNode<rnext.pos, join>{};
			}
			else
			{
				return TreeNode<lnext.pos, lnode>{};
			}
		}

		// starting point to parse everything after the from keyword
		template <std::size_t Pos>
		static constexpr auto parse_from() noexcept
		{
			constexpr auto next{ parse_join<Pos>() };

			using node = typename decltype(next)::node;

			if constexpr (next.pos + 1 < tokens_.count() && (tokens_[next.pos] == "where" || tokens_[next.pos] == "WHERE"))
			{
				using output = std::remove_cvref_t<typename node::output_type>;
				using predicate = typename decltype(parse_or<next.pos + 1, output>())::node;

				return ra::selection<predicate, node>{};
			}
			else
			{
				return node{};	
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

		// searches for the token position of start of the next column in the token array
		template <std::size_t Pos>
		static constexpr auto next_column() noexcept
		{
			if constexpr (tokens_[Pos] == ",")
			{
				return Pos + 1;
			}
			else if constexpr (tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return Pos;
			}
			else
			{
				return next_column<Pos + 1>();
			}
		}

		// search for if the column is renamed, and returns the position of the name change if so
		template <std::size_t Pos, std::size_t Curr>
		static constexpr auto find_rename() noexcept
		{
			if constexpr (tokens_[Pos] == "," || tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return Curr;
			}
			else if constexpr (tokens_[Pos] == "as" || tokens_[Pos] == "AS")
			{
				return Pos + 1;
			}
			else
			{
				return find_rename<Pos + 1, Curr>();
			}
		}

		// parses the column starting from Pos for all it's info (name, type, and pos of next column)
		template <std::size_t Pos, bool Rename>
		static constexpr auto parse_colinfo() noexcept
		{
			constexpr auto next{ next_column<Pos>() };

			if constexpr (Rename)
			{
				constexpr auto offset{ find_rename<Pos, Pos>() };

				using col = decltype(column_type<Pos>());
				
				return ColInfo<col, offset, next>{};
			}
			else
			{
				return ColInfo<std::remove_const_t<decltype(column_type<Pos>())>, Pos, next>{};
			}
		}

		// recursively parse all columns projected/renamed in the query
		template <std::size_t Pos, bool Rename>
		static constexpr auto recurse_columns() noexcept
		{
			if constexpr (tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return TreeNode<Pos, sql::void_row>{};
			}
			else
			{
				constexpr auto info{ parse_colinfo<Pos, Rename>() };
				constexpr cexpr::string<char, tokens_[info.name].length() + 1> name{ tokens_[info.name] };
				constexpr auto child{ recurse_columns<info.next, Rename>() };

				using next = std::remove_const_t<typename decltype(child)::node>;
				using col = std::remove_const_t<decltype(sql::column<name, typename decltype(info)::type>{})>;

				return TreeNode<child.pos, sql::row<col, next>>{};
			}
		}

		// wrapper to parse columns as a projection RA node
		template <std::size_t Pos>
		static constexpr auto parse_projection() noexcept
		{
			constexpr auto projection{ recurse_columns<Pos, false>() };

			using node = typename decltype(projection)::node;
			using next = decltype(parse_from<projection.pos + 1>());

			return ra::projection<node, next>{};
		}

		// wrapper to parse columns as a rename RA node
		template <std::size_t Pos>
		static constexpr auto parse_rename() noexcept
		{
			using node = typename decltype(recurse_columns<Pos, true>())::node;
			using next = decltype(parse_projection<Pos>());

			return ra::rename<node, next>{};
		}

		template <std::size_t Pos>
		static constexpr bool has_rename() noexcept
		{
			if constexpr (tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return false;
			}
			else if constexpr (tokens_[Pos] == "as" || tokens_[Pos] == "AS")
			{
				return true;
			}
			else
			{
				return has_rename<Pos + 1>();
			}
		};

		// decide RA node to root the expression tree
		template <std::size_t Pos>
		static constexpr auto parse_root() noexcept
		{
			if constexpr (tokens_[Pos] == "*")
			{
				return parse_from<Pos + 2>();
			}
			else if constexpr (has_rename<Pos + 1>())
			{
				return parse_projection<Pos>();
			}
			else
			{
				return parse_rename<Pos>();
			}
		}

		static constexpr sql::tokens<char, sql::preprocess(Str)> tokens_{ Str };

		using expression = decltype(parse_root<1>());
	
	public:
		using iterator = query_iterator<expression>;

		constexpr query(Schemas const&... tables)
		{
			expression::seed(tables...);
		}

		~query()
		{
			expression::reset();
		}

		constexpr iterator begin()
		{
			return iterator{ false };
		}

		constexpr iterator end()
		{
			return iterator{ true };
		}
	};

} // namespace sql
