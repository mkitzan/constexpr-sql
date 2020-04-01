#pragma once

#include <type_traits>

#include "cexpr/string.hpp"

#include "sql/row.hpp"

namespace sql
{

	template <cexpr::string... Columns>
	struct index
	{
		template <typename Row>
		struct comp
		{
			bool operator()(Row const& left, Row const& right) const
			{
				return compare<Columns...>(left, right);
			}
		
		private:
			template <cexpr::string Col, cexpr::string... Cols>
			bool compare(Row const& left, Row const& right) const
			{
				auto const& l{ sql::get<Col>(left) };
				auto const& r{ sql::get<Col>(right) };

				if constexpr (sizeof...(Cols) != 0)
				{
					if (l == r)
					{
						return compare<Cols...>(left, right);
					}					
				}
				
				return l < r;
			}
		};
	};

} // namespace sql
