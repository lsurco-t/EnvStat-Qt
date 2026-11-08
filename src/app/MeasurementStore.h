#pragma once
#include "Measurement.h"
#include <deque>
#include <optional>

// Bounded, in-memory history of the measurements received this session.
class MeasurementStore {
	public:
		explicit MeasurementStore(std::size_t maxSize = 1440);

		void add(const Measurement &measurement);
		void clear();

		std::size_t size() const;
		bool empty() const;
		std::optional<Measurement> latest() const;
		const std::deque<Measurement> &all() const;

	private:
		std::size_t maxSize_;
		std::deque<Measurement> data_;
};
