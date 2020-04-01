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
				constexpr auto expr{ parse_or<Pos + 1, Row>() };

				return TreeNode<expr.pos + 1, typename decltype(expr)::node>{};
			}
			else if constexpr (tokens_[Pos] == "\'" || tokens_[Pos] == "\"")
			{
				constexpr cexpr::string<char, tokens_[Pos + 1].length() + 1> name{ tokens_[Pos + 1] };

				return TreeNode<Pos + 3, sql::constant<value<decltype(name)>{ name }, Row>>{};
			}
			else if constexpr (isdigit(tokens_[Pos][0]))
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };
				constexpr auto val{ isintegral(tokens_[Pos]) ? std::int64_t{} : double{} };

				return TreeNode<Pos + 1, sql::constant<sql::convert<std::remove_const_t<decltype(val)>>(name), Row>>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				return TreeNode<Pos + 1, sql::variable<name, Row>>{};
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
				constexpr auto right{ parse_terms<Left::pos + 1, Row>() };
				constexpr cexpr::string<char, tokens_[Left::pos].length() + 1> name{ tokens_[Left::pos] };
				constexpr auto node{ sql::operation<name, Row, typename Left::node, typename decltype(right)::node>{} };

				return TreeNode<right.pos, std::remove_cvref_t<decltype(node)>>{};
			}			
		}

		// descend further and attempt to parse a compare operation
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_comp() noexcept
		{
			constexpr auto left{ parse_terms<Pos, Row>() };
			
			return recurse_comp<decltype(left), Row>();
		}

		// attempt to parse a negation operation then descend further
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_negation() noexcept
		{
			if constexpr (tokens_[Pos] == "not" || tokens_[Pos] == "NOT")
			{
				constexpr auto expr{ parse_comp<Pos + 1, Row>() };

				return TreeNode<expr.pos, sql::operation<"NOT", Row, typename decltype(expr)::node>>{};
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
				constexpr auto right{ parse_negation<Left::pos + 1, Row>() };
				constexpr auto node{ sql::operation<"AND", Row, typename Left::node, typename decltype(right)::node>{} };

				return recurse_and<TreeNode<right.pos, std::remove_cvref_t<decltype(node)>>, Row>();
			}
		}

		// descend further then attempt to parse AND operations
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_and() noexcept
		{
			constexpr auto left{ parse_negation<Pos, Row>() };
			
			return recurse_and<decltype(left), Row>();
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
				constexpr auto right{ parse_and<Left::pos + 1, Row>() };
				constexpr auto node{ sql::operation<"OR", Row, typename Left::node, typename decltype(right)::node>{} };

				return recurse_or<TreeNode<right.pos, std::remove_cvref_t<decltype(node)>>, Row>();
			}
		}

		// descend further then attempt to parse OR operations
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_or() noexcept
		{
			constexpr auto left{ parse_and<Pos, Row>() };
			
			return recurse_or<decltype(left), Row>();
		}

		// find correct schema for terminal relation
		template <std::size_t Pos, std::size_t Id, typename Schema, typename... Others>
		static constexpr auto recurse_schemas()
		{
			if constexpr (Pos == 0)
			{
				return ra::relation<Schema, Id>{};
			}
			else
			{
				return recurse_schemas<Pos - 1, Id, Others...>();
			}
		}

		// wrapper function to determine terminal relation (NOTE: max tables per query is 10 [0-9])
		template <std::size_t Pos>
		static constexpr auto parse_schema()
		{
			return recurse_schemas<tokens_[Pos][1] - '0', Pos, Schemas...>();
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
			if constexpr (Pos + 3 < tokens_.count() && (tokens_[Pos + 2] == "join" || tokens_[Pos + 2] == "JOIN"))
			{
				constexpr auto node{ choose_join<Pos + 1, decltype(parse_schema<Pos>()), decltype(parse_schema<Pos + 3>())>() };

				return TreeNode<Pos + 4, decltype(node)>{};
			}
			else
			{
				return TreeNode<Pos + 1, decltype(parse_schema<Pos>())>{};
			}
		}

		// starting point to parse everything after the from keyword
		template <std::size_t Pos>
		static constexpr auto parse_from() noexcept
		{
			constexpr auto root{ parse_join<Pos>() };

			if constexpr (root.pos + 1 < tokens_.count() && (tokens_[root.pos] == "where" || tokens_[root.pos] == "WHERE"))
			{
				constexpr auto predicate{ parse_or<root.pos + 1, std::remove_cvref_t<typename decltype(root)::node::output_type>>() };
				
				return ra::selection<typename decltype(predicate)::node, typename decltype(root)::node>{};
			}
			else
			{
				return typename decltype(root)::node{};	
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
				
				return ColInfo<decltype(column_type<Pos>()), offset, next>{};
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
				constexpr auto child{ recurse_columns<info.next, Rename>() };
				constexpr cexpr::string<char, tokens_[info.name].length() + 1> name{ tokens_[info.name] };
				constexpr auto col{ sql::column<name, typename decltype(info)::type>{} };

				return TreeNode<child.pos, sql::row<std::remove_const_t<decltype(col)>, std::remove_const_t<typename decltype(child)::node>>>{};
			}
		}

		// wrapper to parse columns as a projection RA node
		template <std::size_t Pos>
		static constexpr auto parse_projection() noexcept
		{
			constexpr auto projection{ recurse_columns<Pos, false>() };

			return ra::projection<typename decltype(projection)::node, decltype(parse_from<projection.pos + 1>())>{};
		}

		// wrapper to parse columns as a rename RA node
		template <std::size_t Pos>
		static constexpr auto parse_rename() noexcept
		{
			constexpr auto rename{ recurse_columns<Pos, true>() };

			return ra::rename<typename decltype(rename)::node, decltype(parse_projection<Pos>())>{};
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
