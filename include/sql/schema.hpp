#pragma once

#include <fstream>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "cexpr/string.hpp"

#include "sql/column.hpp"
#include "sql/index.hpp"
#include "sql/row.hpp"

namespace sql
{

	template <cexpr::string Name, typename Index, typename... Cols>
	class schema
	{
	public:
		static constexpr auto name{ Name };

		using row_type = sql::variadic_row<Cols...>::row_type;
		using container = typename
			std::conditional_t<
				std::is_same_v<Index, sql::index<>>,
				std::vector<row_type>,
				std::multiset<row_type, typename Index::template comparator<row_type>>
			>;
		using const_iterator = typename container::const_iterator;
		
		schema() = default;

		template <typename Type, typename... Types>
		schema(std::vector<Type> const& col, Types const&... cols) : schema{}
		{
			insert(col, cols...);
		}

		template <typename Type, typename... Types>
		schema(std::vector<Type>&& col, Types&&... cols) : schema{}
		{
			insert(std::forward<Type>(col), std::forward<Types>(cols)...);
		}

		template <typename... Types>
		inline void emplace(Types const&... vals)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.emplace_back(vals...);
			}
			else
			{
				table_.emplace(vals...);
			}
		}

		template <typename... Types>
		inline void emplace(Types&&... vals)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.emplace_back(vals...);
			}
			else
			{
				table_.emplace(vals...);
			}
		}

		template <typename Type, typename... Types>
		void insert(std::vector<Type> const& col, Types const&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(col[i], cols[i]...);
			}
		}

		template <typename Type, typename... Types>
		void insert(std::vector<Type>&& col, Types&&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(std::forward<Type>(col[i]), std::forward<Types>(cols[i])...);
			}
		}

		void insert(row_type const& row)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.push_back(row);
			}
			else
			{
				table_.insert(row);
			}
		}

		void insert(row_type&& row)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.push_back(std::forward<row_type>(row));
			}
			else
			{
				table_.insert(std::forward<row_type>(row));
			}
		}

		inline const_iterator begin() const noexcept
		{
			return table_.begin();
		}

		inline const_iterator end() const noexcept
		{
			return table_.end();
		}

	private:
		container table_;
	};

	namespace
	{

		template <typename Row>
		void fill(std::fstream& fstr, Row& row, [[maybe_unused]] char delim)
		{
			if constexpr (!std::is_same_v<Row, sql::void_row>)
			{
				if constexpr (std::is_same_v<typename Row::column::type, std::string>)
				{
					if constexpr (std::is_same_v<typename Row::next, sql::void_row>)
					{
						std::getline(fstr, row.head());
					}
					else
					{
						std::getline(fstr, row.head(), delim);
					}
				}
				else
				{
					fstr >> row.head();
				}

				fill<typename Row::next>(fstr, row.tail(), delim);
			}
		}

		template <typename Row>
		void fill(std::fstream& fstr, Row const& row, char delim)
		{
			if constexpr (!std::is_same_v<Row, sql::void_row>)
			{
				fstr << row.head();

				if constexpr (std::is_same_v<typename Row::next, sql::void_row>)
				{
					fstr << '\n';
				}
				else
				{
					fstr << delim;
				}
				
				fill<typename Row::next>(fstr, row.tail(), delim);
			}
		}

	} // namespace

	// helper function for users to load a data into a schema from a file
	template <typename Schema>
	Schema load(std::string const& file, char delim)
	{
		auto fstr{ std::fstream(file, fstr.in) };
		Schema table{};
		typename Schema::row_type row{};

		while (fstr)
		{
			fill<typename Schema::row_type>(fstr, row, delim);
			table.insert(std::move(row));

			// in case last stream extraction did not remove newline
			if (fstr.get() != '\n')
			{
				fstr.unget();
			}
		}

		return table;
	}

	// for compat with previous versions
	template <typename Schema, char Delim>
	inline Schema load(std::string const& file)
	{
		return load<Schema>(file, Delim);
	}

	// will work with schema and query objects
	template <typename Type>
	void store(Type const& data, std::string const& file, char delim)
	{
		auto fstr{ std::fstream(file, fstr.out) };

		for (auto const& row : data)
		{
			fill<typename Type::row_type>(fstr, row, delim);
		}
	}

	// for devs who want to use the previous format
	template <typename Type, char Delim>
	inline void store(Type const& data, std::string const& file)
	{
		store<Type>(data, file, Delim);
	}

} // namespace sql
