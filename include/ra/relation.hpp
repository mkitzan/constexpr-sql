#pragma once

#include <exception>
#include <type_traits>

namespace ra
{

	struct data_end : std::exception
	{};

	template <typename Schema, std::size_t Id>
	class relation
	{
	public:
		using output_type = Schema::row_type&;

		static auto& next()
		{
			if (curr != end)
			{
				return *curr++;
			}
			else
			{
				throw ra::data_end{};
			}
		}
		
		template <typename Input, typename... Inputs>
		static void seed(Input const& r, Inputs const&... rs)
		{
			if constexpr (std::is_same_v<Input, Schema>)
			{
				curr = r.begin();
				begin = r.begin();
				end = r.end();
			}
			else
			{
				seed(rs...);
			}
		}

		static inline void reset()
		{
			curr = begin;
		}

	private:
		static Schema::const_iterator curr;
		static Schema::const_iterator begin;
		static Schema::const_iterator end;
	};

	template <typename Schema, std::size_t Id>
	Schema::const_iterator relation<Schema, Id>::curr{};

	template <typename Schema, std::size_t Id>
	Schema::const_iterator relation<Schema, Id>::begin{};

	template <typename Schema, std::size_t Id>
	Schema::const_iterator relation<Schema, Id>::end{};

} // namespace ra
