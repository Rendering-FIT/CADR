#pragma once

#include <string>

namespace CadR {


class Error {
protected:
	std::string _whatString;
public:
	const std::string& what() const noexcept  { return _whatString; }
	Error(const std::string& whatString) : _whatString(whatString) {}
};


class OutOfResources : public Error { using Error::Error; };


}
