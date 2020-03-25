#pragma once

template<typename T>
struct Matrix {
	Matrix(int rows, int columns) : 
		_data(std::make_unique<T[]>(rows * columns)), _rows(rows), _columns(columns) {
	}

	int Rows() const {
		return _rows;
	}

	int Columns() const {
		return _columns;
	}

	struct DataRow {
		DataRow(const Matrix<T>& m, int row) : _m(m), _row(row) {
		}

		const T& operator[](int column) const {
			return _m._data[_row * _m.Columns() + column];
		}

		T& operator[](int column) {
			return _m._data[_row * _m.Columns() + column];
		}

	private:
		const Matrix<T>& _m;
		int _row;
	};

	const DataRow operator[](int row) const {
		const DataRow dr(*this, row);
		return dr;
	}

	DataRow operator[](int row) {
		DataRow dr(*this, row);
		return dr;
	}

private:
	std::unique_ptr<T[]> _data;
	int _columns, _rows;
};
