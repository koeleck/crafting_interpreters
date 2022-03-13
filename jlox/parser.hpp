#pragma once

#include <span>
#include <string_view>

class BumpAlloc;
struct ScannerResult;
class Expr;
class Stmt;

Expr* parse(BumpAlloc& alloc, const ScannerResult& scanner_result);
