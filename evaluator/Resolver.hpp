#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include <stdexcept>
#include <string>

#include "AST.hpp"
#include "Timeline.hpp"

class ResolveError : public std::runtime_error {
public:
    explicit ResolveError(const std::string& message) : std::runtime_error(message) {}
};

Timeline resolve(const ProgramAST& program);

#endif
