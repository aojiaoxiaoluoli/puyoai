#include "rensa_detector.h"

#include <iostream>
#include <set>
#include <vector>

#include "core/algorithm/column_puyo_list.h"
#include "core/algorithm/plan.h"
#include "core/algorithm/rensa_info.h"
#include "core/algorithm/rensa_ref_sequence.h"
#include "core/decision.h"
#include "core/field/core_field.h"
#include "core/kumipuyo.h"

using namespace std;

namespace {

typedef std::function<void (CoreField*, const ColumnPuyoList&)> SimulationCallback;

};

static inline void tryDropFire(const CoreField& originalField, SimulationCallback callback)
{
    bool visited[CoreField::MAP_WIDTH][NUM_PUYO_COLORS] {};

    for (int x = 1; x <= CoreField::WIDTH; ++x) {
        for (int y = originalField.height(x); y >= 1; --y) {
            PuyoColor c = originalField.color(x, y);

            if (!isNormalColor(c))
                continue;

            // Drop puyo on
            for (int d = -1; d <= 1; ++d) {
                if (visited[x + d][c])
                    continue;

                if (x + d <= 0 || CoreField::WIDTH < x + d)
                    continue;
                if (d == 0) {
                    if (originalField.color(x, y + 1) != PuyoColor::EMPTY)
                        continue;
                } else {
                    if (originalField.color(x + d, y) != PuyoColor::EMPTY)
                        continue;
                }

                visited[x + d][c] = true;

                CoreField f(originalField);
                int necessaryPuyos = 0;
                while (necessaryPuyos <= 4 && f.countConnectedPuyos(x, y) < 4 && f.height(x + d) <= 12) {
                    f.dropPuyoOn(x + d, c, true);
                    ++necessaryPuyos;
                }

                if (necessaryPuyos > 4)
                    continue;

                callback(&f, ColumnPuyoList(x + d, c, necessaryPuyos));
            }
        }
    }
}

static inline void tryFloatFire(const CoreField& originalField, SimulationCallback callback)
{
    for (int x = 1; x <= CoreField::WIDTH; ++x) {
        for (int y = originalField.height(x); y >= 1; --y) {
            PuyoColor c = originalField.color(x, y);

            DCHECK(c != PuyoColor::EMPTY);
            if (c == PuyoColor::OJAMA)
                continue;

            int necessaryPuyos = 4 - originalField.countConnectedPuyos(x, y);
            int restPuyos = necessaryPuyos;
            CoreField f(originalField);

            int dx = x - 1;
            // float puyo col dx
            for (; dx <= x + 1 && restPuyos > 0; ++dx) {
                if (dx <= 0 || CoreField::WIDTH < dx) {
                    continue;
                }


                // Check y
                if (dx != x) {
                    if (originalField.color(dx, y) != PuyoColor::EMPTY) {
                        continue;
                    } else { // restPuyos must be more than 0
                        f.unsafeSet(dx, y, c);
                        --restPuyos;
                    }
                }

                int dy_min = y - 1;
                // Check under y
                for (; restPuyos > 0 && dy_min > 0 && originalField.color(dx ,dy_min) == PuyoColor::EMPTY;
                     --dy_min) {
                    f.unsafeSet(dx, dy_min, c);
                    --restPuyos;
                }

                // Check over y
                for (int dy = y + 1;
                     restPuyos > 0 && dy <= 12 && originalField.color(dx ,dy) == PuyoColor::EMPTY; ++dy) {
                    f.unsafeSet(dx, dy, c);
                    --restPuyos;
                }

                // Fill ojama
                for(; dy_min > 0 && originalField.color(dx, dy_min) == PuyoColor::EMPTY; --dy_min) {
                    f.unsafeSet(dx, dy_min, PuyoColor::OJAMA);
                }

                f.recalcHeightOn(dx);
            }

            if (restPuyos <= 0) {
                callback(&f, ColumnPuyoList(dx, c, necessaryPuyos));
            }
        }
    }
}

static
void findRensas(const CoreField& field, RensaDetector::Mode mode, SimulationCallback callback)
{
    switch (mode) {
    case RensaDetector::Mode::DROP:
        tryDropFire(field, callback);
        break;
    case RensaDetector::Mode::FLOAT:
        tryFloatFire(field, callback);
        break;
    default:
        CHECK(false) << "Unknown mode : " << static_cast<int>(mode);
    }
}


std::vector<FeasibleRensaInfo>
RensaDetector::findFeasibleRensas(const CoreField& field, const KumipuyoSeq& kumipuyoSeq)
{
    std::vector<FeasibleRensaInfo> result;
    Plan::iterateAvailablePlans(field, kumipuyoSeq, kumipuyoSeq.size(), [&result](const RefPlan& plan) {
        if (plan.isRensaPlan())
            result.emplace_back(plan.rensaResult(), plan.initiatingFrames());
    });

    return result;
}

static inline void simulateInternal(CoreField* f, const CoreField& original,
                                    const ColumnPuyoList& keyPuyos, const ColumnPuyoList& firePuyos,
                                    RensaDetector::PossibleRensaCallback callback)
{
    int minHeights[CoreField::MAP_WIDTH] {
        100, original.height(1) + 1, original.height(2) + 1, original.height(3) + 1,
        original.height(4) + 1, original.height(5) + 1, original.height(6) + 1, 100,
    };

    RensaResult rensaResult = f->simulateWithMinHeights(minHeights);
    callback(*f, rensaResult, keyPuyos, firePuyos);
}

static inline void simulateInternal(CoreField* f, const CoreField& original,
                                    const ColumnPuyoList& keyPuyos, const ColumnPuyoList& firePuyos,
                                    RensaDetector::TrackedPossibleRensaCallback callback)
{
    int minHeights[CoreField::MAP_WIDTH] {
        100, original.height(1) + 1, original.height(2) + 1, original.height(3) + 1,
        original.height(4) + 1, original.height(5) + 1, original.height(6) + 1, 100,
    };

    RensaTrackResult rensaTrackResult;
    RensaResult rensaResult = f->simulateAndTrackWithMinHeights(&rensaTrackResult, minHeights);
    callback(*f, rensaResult, keyPuyos, firePuyos, rensaTrackResult);
}

template<typename Callback>
static void findPossibleRensasInternal(const CoreField& originalField,
                                       const ColumnPuyoList& keyPuyos,
                                       int leftX,
                                       int restAdded,
                                       RensaDetector::Mode mode,
                                       Callback callback)
{
    auto findRensaCallback = [&originalField, &keyPuyos, &callback](CoreField* f, const ColumnPuyoList& firePuyos) {
        simulateInternal(f, originalField, keyPuyos, firePuyos, callback);
    };

    findRensas(originalField, mode, findRensaCallback);

    if (restAdded <= 0)
        return;

    CoreField f(originalField);
    ColumnPuyoList puyoList(keyPuyos);

    for (int x = leftX; x <= CoreField::WIDTH; ++x) {
        if (f.height(x) >= 12)
            continue;

        for (int i = 0; i < NUM_NORMAL_PUYO_COLORS; ++i) {
            PuyoColor c = NORMAL_PUYO_COLORS[i];

            if (!f.dropPuyoOn(x, c, true))
                continue;
            puyoList.addPuyo(x, c);

            if (f.countConnectedPuyos(x, f.height(x)) < 4)
                findPossibleRensasInternal(f, puyoList, x, restAdded - 1, mode, callback);

            f.removeTopPuyoFrom(x);
            puyoList.removeLastAddedPuyo();
        }
    }
}

void RensaDetector::iteratePossibleRensas(const CoreField& field, int maxKeyPuyos,
                                          RensaDetector::PossibleRensaCallback callback, RensaDetector::Mode mode)
{
    ColumnPuyoList puyoList;
    findPossibleRensasInternal(field, puyoList, 1, maxKeyPuyos,mode, callback);
}

void RensaDetector::iteratePossibleRensasWithTracking(const CoreField& field, int maxKeyPuyos,
                                                      RensaDetector::TrackedPossibleRensaCallback callback, RensaDetector::Mode mode)
{
    ColumnPuyoList puyoList;
    findPossibleRensasInternal(field, puyoList, 1, maxKeyPuyos, mode, callback);
}

static
void iteratePossibleRensasIterativelyInternal(const CoreField& originalField, const CoreField& initialField, int restIterations,
                                              const ColumnPuyoList& accumulatedKeyPuyos, const ColumnPuyoList& firstRensaFirePuyos,
                                              RensaRefSequence* rensaSequence,
                                              const RensaDetector::IterativePossibleRensaCallback& callback,
                                              RensaDetector::Mode mode)
{
    if (restIterations <= 0)
        return;

    auto findRensaCallback = [&](CoreField* f, const ColumnPuyoList& currentFirePuyos) {
        auto simulationCallback = [&](const CoreField& fieldAfterSimulation, const RensaResult& rensaResult,
                                      const ColumnPuyoList& currentKeyPuyos, const ColumnPuyoList& currentFirePuyos,
                                      const RensaTrackResult& trackResult) {
            ColumnPuyoList combinedKeyPuyos(accumulatedKeyPuyos);
            combinedKeyPuyos.append(currentFirePuyos);

            // Here, try to fire the combined rensa.
            {
                CoreField f(initialField);
                for (const ColumnPuyo& cp : combinedKeyPuyos) {
                    // When we cannot put a puyo, that rensa is broken.
                    if (!f.dropPuyoOn(cp.x, cp.color, true))
                        return;
                }

                // Check putting key puyo does not fire a rensa.
                {
                    int minHeights[CoreField::MAP_WIDTH] {
                        100, initialField.height(1) + 1, initialField.height(2) + 1, initialField.height(3) + 1,
                        initialField.height(4) + 1, initialField.height(5) + 1, initialField.height(6) + 1, 100 };
                    RensaResult tempRensaResult = f.simulateWithMinHeights(minHeights);
                    if (tempRensaResult.chains > 0) {
                        // Rensa should not start when we add key puyos.
                        return;
                    }
                }

                // Since key puyo does not fire a rensa, we can safely include the key puyos in minHeights.
                int minHeights[CoreField::MAP_WIDTH] {
                    100, f.height(1) + 1, f.height(2) + 1, f.height(3) + 1,
                    f.height(4) + 1, f.height(5) + 1, f.height(6) + 1, 100 };

                // Then, fire a rensa.
                for (const ColumnPuyo& cp : firstRensaFirePuyos) {
                    if (!f.dropPuyoOn(cp.x, cp.color, true))
                        return;
                }

                RensaTrackResult combinedTrackResult;
                RensaResult combinedRensaResult = f.simulateAndTrackWithMinHeights(&combinedTrackResult, minHeights);

                if (combinedRensaResult.chains != rensaSequence->totalChains() + rensaResult.chains) {
                    // Rensa looks broken. We don't count such rensa.
                    return;
                }

                // OK.
                RensaRef rensaRef { originalField, fieldAfterSimulation, currentKeyPuyos, currentFirePuyos, rensaResult, trackResult };
                rensaSequence->push(&rensaRef);

                callback(f, combinedRensaResult, combinedKeyPuyos, firstRensaFirePuyos, combinedTrackResult, *rensaSequence);
                iteratePossibleRensasIterativelyInternal(fieldAfterSimulation, initialField, restIterations - 1,
                                                         combinedKeyPuyos, firstRensaFirePuyos, rensaSequence, callback, mode);

                rensaSequence->pop();
            }

        };

        simulateInternal(f, originalField, ColumnPuyoList(), currentFirePuyos, simulationCallback);
    };

    findRensas(originalField, mode, findRensaCallback);
}

// iteratePossibleRensasIteratively finds rensa with the following algorithm.
// 1. First, iterate all rensas (first rensa) from the current field.
// 2. For each field after the rensa, we find another rensa (second rensa).
// 3. We think the necessary puyos to fire the second rensa is the key puyos for the first rensa.
//    Add the key puyos to the original field, and try to fire a rensa. If the first rensa and the second
//    rensa can be combined, we think the combined rensa as the whole rensa.
//    Otherwise, we think it is broken.
void RensaDetector::iteratePossibleRensasIteratively(const CoreField& originalField, int maxIteration,
                                                     IterativePossibleRensaCallback callback, Mode mode)
{
    RensaRefSequence rensaSequence;

    auto findRensaCallback = [&](CoreField* f, const ColumnPuyoList& firePuyos) {
        auto simulationCallback = [&](const CoreField& fieldAfterSimulation, const RensaResult& rensaResult,
                                      const ColumnPuyoList& keyPuyos, const ColumnPuyoList& firePuyos,
                                      const RensaTrackResult& trackResult) {
            RensaRef ref { originalField, fieldAfterSimulation, keyPuyos, firePuyos, rensaResult, trackResult };
            rensaSequence.push(&ref);

            callback(fieldAfterSimulation, rensaResult, keyPuyos, firePuyos, trackResult, rensaSequence);

            iteratePossibleRensasIterativelyInternal(fieldAfterSimulation, originalField, maxIteration - 1,
                                                     ColumnPuyoList(), firePuyos, &rensaSequence, callback, mode);
            rensaSequence.pop();
        };

        simulateInternal(f, originalField, ColumnPuyoList(), firePuyos, simulationCallback);
    };

    findRensas(originalField, mode, findRensaCallback);
}
