#pragma once

#include <vector>

namespace dynamix {

	template <typename T>
	class Stack
	{
	public:
		Stack() = default;
		~Stack() = default;

		void push(T value) {
			m_Data.push_back(value);
		}

		T& pop() {
			T& back = m_Data.back();
			m_Data.pop_back();
			return back;
		}

		size_t size() const {
			return m_Data.size();
		}

		T* first() {
			return m_Data.data();
		}

		const T* first() const {
			return m_Data.data();
		}

		T* top() {
			return &m_Data[m_Data.size()];
		}

		const T* top() const {
			return &m_Data[m_Data.size()];
		}

		bool is_empty() const {
			return m_Data.empty();
		}

		void clear() {
			m_Data.clear();
		}

		void resize(size_t new_size) {
			m_Data.resize(new_size);
		}

		void reserve(size_t new_capacity) {
			m_Data.reserve(new_capacity);
		}

		T& operator[](int32_t index) {
			return m_Data[index];
		}

		const T& operator[](int32_t index) const {
			return m_Data[index];
		}

	private:
		std::vector<T> m_Data;
	};

}
