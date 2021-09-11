#pragma once

#include <stdexcept>
#include <string>

namespace CadR {


class Error {
public:
	virtual const char* what() const noexcept = 0;
};


class LogicError : public Error, public std::logic_error {
public:
	virtual const char* what() const noexcept override  { return std::logic_error::what(); }
	LogicError(const char* what) : Error(), std::logic_error(what)  {}
	LogicError(const std::string& what) : Error(), std::logic_error(what)  {}
};


class OutOfResources : public Error, public std::runtime_error {
public:
	virtual const char* what() const noexcept override  { return std::runtime_error::what(); }
	OutOfResources(const char* what) : Error(), std::runtime_error(what)  {}
	OutOfResources(const std::string& what) : Error(), std::runtime_error(what)  {}
};


}
