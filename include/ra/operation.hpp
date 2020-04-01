#pragma once

#include <type_traits>

namespace ra
{

	template <typename Input>
	class unary
	{
	public:
		using input_type = std::remove_cvref_t<decltype(Input::next())>;

		template <typename... Inputs>
		static inline void seed(Inputs const&... rs)
		{
			Input::seed(rs...);
		}

		static inline void reset()
		{
			Input::reset();
		}
	};

	template <typename LeftInput, typename RightInput>
	class binary
	{
	public:
		using left_type = std::remove_cvref_t<decltype(LeftInput::next())>;
		using right_type = std::remove_cvref_t<decltype(RightInput::next())>;

		template <typename... Inputs>
		static inline void seed(Inputs const&... rs)
		{
			LeftInput::seed(rs...);
			RightInput::seed(rs...);
		}

		static inline void reset()
		{
			LeftInput::reset();
			RightInput::reset();
		}
	};

} // namespace ra
