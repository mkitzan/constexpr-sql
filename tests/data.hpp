#pragma once

#include <string>
#include <type_traits>

#include "sql.hpp"

using books =
	sql::schema<
		sql::index<>,
#ifdef CROSS
		sql::column<"book", std::string>,
#else
		sql::column<"title", std::string>,
#endif
		sql::column<"genre", std::string>,
		sql::column<"year", unsigned>,
		sql::column<"pages", unsigned>
	>;

using stories =
	sql::schema<
		sql::index<>,
#ifdef CROSS
		sql::column<"story", std::string>,
#else
		sql::column<"title", std::string>,
#endif
		sql::column<"genre", std::string>,
		sql::column<"year", unsigned>
	>;

using authored =
	sql::schema<
		sql::index<>,
		sql::column<"title", std::string>,
		sql::column<"name", std::string>
	>;

using collected =
	sql::schema<
		sql::index<>,
		sql::column<"title", std::string>,
		sql::column<"collection", std::string>,
		sql::column<"pages", unsigned>
	>;

const std::string data_folder{ "./data/" };
const std::string perf_folder{ "../data/" };
const std::string books_data{ "books-table.tsv" };
const std::string stories_data{ "stories-table.tsv" };
const std::string authored_data{ "authored-table.tsv" };
const std::string collected_data{ "collected-table.tsv" };

using books_row = std::tuple<std::string, std::string, int, int>;
using books_type = std::vector<books_row>;
using stories_row = std::tuple<std::string, std::string, int>;
using stories_type = std::vector<stories_row>;
using authored_row = std::tuple<std::string, std::string>;
using authored_type = std::vector<authored_row>; 
using collected_row = std::tuple<std::string, std::string, int>;
using collected_type = std::vector<collected_row>;

constexpr std::size_t iters{ 65536 };
constexpr std::size_t offset{ 512 };

template <char Delim>
books_type books_load()
{
	auto file{ std::fstream(perf_folder + books_data) };
	books_type table{};

	while (file)
	{
		books_row row{};
		std::getline(file, std::get<0>(row), Delim);
		std::getline(file, std::get<1>(row), Delim);
		file >> std::get<2>(row);
		file >> std::get<3>(row);
		
		table.push_back(std::move(row));

		if (file.get() != '\n')
		{
			file.unget();
		}
	}

	return table;	
}

template <char Delim>
stories_type stories_load()
{
	auto file{ std::fstream(perf_folder + stories_data) };
	stories_type table{};

	while (file)
	{
		stories_row row{};
		std::getline(file, std::get<0>(row), Delim);
		std::getline(file, std::get<1>(row), Delim);
		file >> std::get<2>(row);
		
		table.push_back(std::move(row));

		if (file.get() != '\n')
		{
			file.unget();
		}
	}

	return table;	
}

template <char Delim>
authored_type authored_load()
{
	auto file{ std::fstream(perf_folder + authored_data) };
	authored_type table{};

	while (file)
	{
		authored_row row{};
		std::getline(file, std::get<0>(row), Delim);
		std::getline(file, std::get<1>(row), Delim);
		
		table.push_back(std::move(row));

		if (file.get() != '\n')
		{
			file.unget();
		}
	}

	return table;	
}

template <char Delim>
collected_type collected_load()
{
	auto file{ std::fstream(perf_folder + collected_data) };
	collected_type table{};

	while (file)
	{
		collected_row row{};
		std::getline(file, std::get<0>(row), Delim);
		std::getline(file, std::get<1>(row), Delim);
		file >> std::get<2>(row);
		
		table.push_back(std::move(row));

		if (file.get() != '\n')
		{
			file.unget();
		}
	}

	return table;	
}
