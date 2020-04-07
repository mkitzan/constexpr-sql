#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../../data.hpp"

stories_type query(stories_type const& s)
{
	using std::get;
	stories_type output{};

	for (auto const& row : s)
	{
		if (!(get<1>(row) != "science fiction") && !(get<2>(row) <= 1970))
		{
			output.push_back(row);
		}
	}

	return output;
}

int main()
{
	stories_type s{ stories_load<'\t'>() };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (auto data{ query(s) }; auto const& [t, g, y] : data)
		{
			std::cout << t << '\t' << g << '\t' << y << '\n';
		}
	}

	return 0;
}
