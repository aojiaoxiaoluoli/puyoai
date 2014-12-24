#ifndef CPU_MAYAH_PATTERN_BOOK_H_
#define CPU_MAYAH_PATTERN_BOOK_H_

#include <string>
#include <toml/toml.h>

#include "base/noncopyable.h"
#include "core/algorithm/column_puyo_list.h"
#include "core/algorithm/pattern_field.h"
#include "core/core_field.h"

class ColumnPuyoList;

class PatternBookField {
public:
    explicit PatternBookField(const std::string& field) : patternField_(field) {}
    explicit PatternBookField(const std::vector<std::string>& field) : patternField_(field) {}

    bool complement(const CoreField&, ColumnPuyoList* cpl);

private:
    PatternField patternField_;
};

class PatternBook : noncopyable {
public:
    bool load(const std::string& filename);
    bool loadFromValue(const toml::Value&);

private:
    std::vector<PatternBookField> fields_;
};

#endif