#pragma once

#include <span>
#include <string_view>
#include <vector>

class BumpAlloc;
struct ScannerResult;
class Expr;
class Stmt;

std::vector<Stmt*> parse(BumpAlloc& alloc, const ScannerResult& scanner_result);
