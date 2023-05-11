#pragma once

namespace dynamix {

	template <typename T>
	struct Maybe
	{
	public:
		using ReferenceType = T&;
		using PointerType = T*;

	public:
		Maybe(bool some, PointerType data)
			: m_Some(some), m_Data(data) { }
		Maybe(bool some)
			: m_Some(some), m_Data(nullptr) { }

		constexpr bool is_some() const {
			return m_Some;
		}

		constexpr ReferenceType data() {
			return *m_Data;
		}

	private:
		const bool m_Some = false;
		PointerType m_Data = nullptr;
	};

}
