#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../../data.hpp"

using output_type = std::vector<std::tuple<std::string, std::string, int, std::string, std::string, int>>;

output_type query(stories_type const& s, collected_type const& c)
{
	using std::get;
	output_type output{};
	
	for (auto const& c_row : c)
	{
		for (auto const& s_row : s)
		{
			if (!(get<1>(s_row) != "science fiction") || get<2>(s_row) >= 1970 || !(get<2>(c_row) < 300))
			{
				output.emplace_back(get<0>(s_row), get<1>(s_row), get<2>(s_row), get<0>(c_row), get<1>(c_row), get<2>(c_row));
			}
		}
	}

	return output;
}

int main()
{
	stories_type s{ stories_load<'\t'>() };
	collected_type c{ collected_load<'\t'>() };

	for (std::size_t i{}; i < iters / 128; ++i)
	{
		for (auto data{ query(s, c) }; auto const& [st, g, y, t, co, p] : data)
		{
			std::cout << st << '\t' << g << '\t' << y << '\t' << t << '\t' << co << '\t' << p << '\n';
		}
	}

	return 0;
}
