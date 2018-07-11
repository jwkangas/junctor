/*
 *  Data class for reading and writing data
 *  
 *  Copyright 2011-2015 Teppo Niinimäki <teppo.niinimaki(at)helsinki.fi>
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstddef>
#include <exception>
#include <boost/format.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include "common.hpp"

#ifndef DATA_HPP
#define DATA_HPP

/*class DataReadException : public Exception {
public:
	DataReadException(const string& msg) : Exception(msg) {
	}
};*/

template <typename T>
class Data {
public:
	enum Type {
		VARWISE = false, RECWISE = true
	};

protected:
	int nVariables_;
	int nRecords_;
	int varSpacing_;
	int recSpacing_;
	Type type_;
	std::vector<T> data;
	
private:
	Data& operator=(const Data&); // disable assignment

public:

	Data(Type type = RECWISE) :
		nVariables_(0),
		nRecords_(0),
		varSpacing_(type == RECWISE ? 1 : nRecords_),
		recSpacing_(type == RECWISE ? nVariables_ : 1),
		type_(type),
		data()
	{
	}
	
	Data(int nVariables, int nRecords, Type type = RECWISE) :
		nVariables_(nVariables),
		nRecords_(nRecords),
		varSpacing_(type == RECWISE ? 1 : nRecords_),
		recSpacing_(type == RECWISE ? nVariables_ : 1),
		type_(type),
		data(nVariables * nRecords)
	{
		assert(nVariables >= 0 && nRecords >= 0);
	}
	
	Data(const Data& other) :
		nVariables_(other.nVariables),
		nRecords_(other.nRecords),
		varSpacing_(other.varSpacing_),
		recSpacing_(other.recSpacing_),
		type_(other.type_),
		data(other.data)
	{
	}

	Data(Data&& other) :
		nVariables_(std::move(other.nVariables)),
		nRecords_(std::move(other.nRecords)),
		varSpacing_(std::move(other.varSpacing_)),
		recSpacing_(std::move(other.recSpacing_)),
		type_(std::move(other.type_)),
		data(std::move(other.data))
	{
	}
	
	virtual void clear() {
		nVariables_ = 0;
		nRecords_ = 0;
		varSpacing_ = 0;
		recSpacing_ = 0;
		data.clear();
	}
	
	virtual ~Data() {
	}
	
	int getNumRecords() const {
		return nRecords_;
	}
	int getNumSamples() const {
		return getNumRecords();
	}
	
	int getNumVariables() const {
		return nVariables_;
	}
	
	Type getType() const {
		return type_;
	}
	/*void setRowWise() {
		return type_;
	}*/
	
	T& operator()(int v, int r) {
		return data[v * varSpacing_ + r * recSpacing_];
	}
	
	const T& operator()(int v, int r) const {
		return data[v * varSpacing_ + r * recSpacing_];
	}
	
	
	void resize(int nVariables, int nRecords) {
		assert(nVariables >= 0 && nRecords >= 0);
		nVariables_ = nVariables;
		nRecords_ = nRecords;
		if (type_ == RECWISE) {
			varSpacing_ = 1;
			recSpacing_ = nVariables;
		}
		else {
			varSpacing_ = nRecords;
			recSpacing_ = 1;
		}
		data.resize(nVariables * nRecords);
	}
	
	template <typename Range>
	void addRecord(Range record) {
		assert(record.size() == nVariables_);
		assert(type_ == RECWISE);
		data.insert(data.end(), record.begin(), record.end());
		++nRecords_;
	}

	template <typename Range>
	void addVariable(Range variable) {
		assert(variable.size() == nRecords_);
		assert(type_ == VARWISE);
		data.insert(data.end(), variable.begin(), variable.end());
		++nVariables_;
	}
	
	template <typename Range>
	void addRow(Range row) {
		if (type_ == RECWISE)
			addRecord(row);
		else
			addVariable(row);
	}
};


template <typename T>
void readData(std::istream& stream, int nVariables, int nRecords, Data<T>* data) {
	data->resize(nVariables, nRecords);
	
	// load the data
	std::string row;
	for (int r = 0; r < nRecords; ++r) {
		if (stream.eof())
			throw Exception("Not enough rows (%d while %d expected).") % r % nRecords;
		getline(stream, row);
		std::istringstream rowStream(row);
		for (int v = 0; v < nVariables; ++v) {
			T tmp;
			rowStream >> tmp;
			if (rowStream.fail())
				throw Exception("Could not read value on row %d column %d") % (r+1) % (v+1);
			(*data)(v, r) = (T) tmp;
		}
	}
}

template <typename T>
std::unique_ptr<Data<T>> readData(std::istream& stream, int nVariables, int nRecords) {
	auto data = std::unique_ptr<Data<T>>(new Data<T>());
	readData(stream, nVariables, nRecords, data.get());
	return data;
}

template <typename T>
void readData(std::istream& stream, Data<T>* data) {
	std::string row;
	std::vector<T> record;
	
	// read the first row (and get the number of variables)
	getline(stream, row);
	std::istringstream rowStream(row);
	rowStream >> std::ws;
	while (!rowStream.eof()) {
		T tmp;
		rowStream >> tmp;
		if (rowStream.fail())
			throw Exception("Could not read value on row 1 column %d.")
					% (record.size() + 1);
		record.push_back(tmp);
		rowStream >> std::ws;
	}
	stream >> std::ws;
	int nVariables = record.size();
	data->resize(nVariables, 0);
	data->addRecord(record);
	
	// load the data rest of the data
	while (!stream.eof()) {
		record.clear();
		getline(stream, row);
		std::istringstream rowStream(row);
		for (int v = 0; v < nVariables; ++v) {
			T tmp;
			rowStream >> tmp;
			if (rowStream.fail())
				throw Exception("Could not read %dth value on row %d")
						% (v + 1) % (data-> getNumRecords() + 1);
			record.push_back(tmp);
		}
		stream >> std::ws;
		data->addRecord(record);
	}
}

template <typename T>
std::unique_ptr<Data<T>> readData(std::istream& stream) {
	auto data = std::unique_ptr<Data<T>>(new Data<T>());
	readData(stream, data.get());
	return data;
}





template <typename T>
class CategoricalData : public Data<T> {
private:
	CategoricalData(const CategoricalData&); // disable copying
	CategoricalData& operator=(const CategoricalData&); // disable copying

	std::vector<int> arities_;
	
	using Data<T>::nVariables_;
	using Data<T>::nRecords_;
	
public:
	using typename Data<T>::Type;
	using Data<T>::RECWISE;
	using Data<T>::VARWISE;
	
	CategoricalData(Type type = RECWISE) :
		Data<T>(type)
	{
	}

	void clear() {
		clear();
		arities_.clear();
	}

	void detectArities() {
		arities_.resize(nVariables_);
		for (int v = 0; v < nVariables_; ++v) {
			int arity = 0;
			for (int r = 0; r < nRecords_; ++r) {
				if ((*this)(v,r) >= arity)
					arity = (*this)(v,r) + 1;
			}
			arities_[v] = arity;
		}
	}
	
	template <typename Range>
	void setArities(Range arities) {
		assert(arities.size() == nVariables_);
		arities_.assign(arities.begin(), arities.end());
	}
	
	int getArity(int v) const {
		assert(arities_.size() == nVariables_);
		return arities_[v];
	}
	
	const std::vector<int>& getArities() {
		assert(arities_.size() == nVariables_);
		return arities_;
	}
};


#endif

