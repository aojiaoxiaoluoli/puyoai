#ifndef CPU_MAYAH_PATTERN_RENSA_DETECTOR_H_
#define CPU_MAYAH_PATTERN_RENSA_DETECTOR_H_

#include <string>
#include <unordered_set>

#include "core/column_puyo_list.h"
#include "core/core_field.h"

#include "pattern_book.h"

class PatternRensaDetector {
public:
    typedef std::function<void (const CoreField&,
                                const RensaResult&,
                                const ColumnPuyoList& keyPuyos,
                                const ColumnPuyoList& firePuyos,
                                const RensaTrackResult&,
                                const std::string& patternName,
                                double patternScore)> Callback;

    PatternRensaDetector(const PatternBook& patternBook,
                         const CoreField& originalField,
                         Callback callback) :
        patternBook_(patternBook),
        originalField_(originalField),
        callback_(std::move(callback)),
        originalContext_(CoreField::SimulationContext::fromField(originalField)),
        strategy_(RensaDetectorStrategy(RensaDetectorStrategy::Mode::DROP, 2, 2, false))
    {
    }

    void iteratePossibleRensas(const std::vector<int>& matchableIds,
                               int maxIteration);

private:
    void iteratePossibleRensasInternal(const CoreField& currentField,
                                       const CoreField::SimulationContext& currentFieldContext,
                                       const RensaYPositionTracker& currentFieldTracker,
                                       int currentChains,
                                       const ColumnPuyo& firePuyo,
                                       const ColumnPuyoList& keyPuyos,
                                       int restIteration,
                                       int restUnusedVariables,
                                       const std::string& patternName,
                                       double currentPatternScore);

    bool checkDup(const ColumnPuyo& firePuyo,
                  const ColumnPuyoList& keyPuyos);

    bool checkRensa(int currentChains,
                    const ColumnPuyo& firePuyo,
                    const ColumnPuyoList& keyPuyos,
                    double patternScore,
                    const std::string& patternName,
                    bool prohibits[FieldConstant::MAP_WIDTH]);

    const PatternBook& patternBook_;
    const CoreField& originalField_;
    Callback callback_;
    const CoreField::SimulationContext originalContext_;
    const RensaDetectorStrategy strategy_;

    std::unordered_set<ColumnPuyoList> usedSet_;
};

#endif