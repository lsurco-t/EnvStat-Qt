#include "MeasurementStore.h"

MeasurementStore::MeasurementStore(std::size_t maxSize)
	: maxSize_(maxSize == 0 ? 1 : maxSize)
{
}

void MeasurementStore::add(const Measurement &measurement){
	data_.push_back(measurement);

	while (data_.size() > maxSize_){
		data_.pop_front();
	}
}

void MeasurementStore::clear(){
	data_.clear();
}

std::size_t MeasurementStore::size() const {
	return data_.size();
}

bool MeasurementStore::empty() const {
	return data_.empty();
}

std::optional<Measurement> MeasurementStore::latest() const {
	if (data_.empty()){
		return std::nullopt;
	}
	return data_.back();
}

const std::deque<Measurement> &MeasurementStore::all() const {
	return data_;
}
