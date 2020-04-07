#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../data.hpp"

using stories_row = std::tuple<std::string, std::string, int>;
using stories_type = std::vector<stories_row>;

template <char Delim>
Schema tuple_load(std::string const& data)
{
	using std::get;
	auto file{ std::fstream(data) };
	stories_type table{};

	while (file)
	{
		stories_row row{};
		std::getline(file, get<0>(row), Delim);
		std::getline(file, get<1>(row), Delim);
		file >> get<2>(row);
		
		table.push_back(std::move(row));

		if (file.get() != '\n')
		{
			file.unget();
		}
	}

	return table;	
}

stories_type query(stories_type const& s)
{
	using std::get;
	stories_type output{};

	for (auto const& row : b)
	{
		auto it{ cache.find(get<0>(row)) };

		if (it != cache.end() && !(get<1>(get<1>(*it)) != "science fiction") && !(get<2>(get<1>(get<1>(*it)) <= 1970))
		{
			output.push_back(row);
		}
	}

	return output;
}

int main()
{
	stories_type s{ tuple_load<'t'>(bench_folder + stories_data) };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (auto data{ query(s) }; auto const& [t, g, y] : data)
		{
			std::cout << t << '\t' << g << '\t' << y << '\n';
		}
	}

	return 0;
}
