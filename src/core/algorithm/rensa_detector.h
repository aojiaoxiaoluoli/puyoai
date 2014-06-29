#ifndef CORE_ALGORITHM_RENSA_DETECTOR_H_
#define CORE_ALGORITHM_RENSA_DETECTOR_H_

#include <vector>

#include "core/field/core_field.h"
#include "core/algorithm/puyo_set.h"
#include "core/algorithm/rensa_info.h"

class KumipuyoSeq;

class RensaDetector {
public:
    enum Mode {
      DROP,
      FLOAT,
    };

    // Finds rensa using |kumiPuyos|.
    static std::vector<FeasibleRensaInfo> findFeasibleRensas(const CoreField&, const KumipuyoSeq&);

    // Finds possible rensas from the specified field.
    // Before firing a rensa, we will put key puyos (maxKeyPuyo at max).
    static std::vector<PossibleRensaInfo> findPossibleRensas(
        const CoreField&, int maxKeyPuyo = 0, Mode mode = DROP);

    // Same as findPossibleRensas. However, we return tracking info also.
    static std::vector<TrackedPossibleRensaInfo> findPossibleRensasWithTracking(
        const CoreField&, int maxKeyPuyo = 0, Mode mode = DROP);

    template<typename SimulationCallback>
    static void findRensas(const CoreField&, Mode mode, SimulationCallback callback);
};

#include "rensa_detector_inl.h"

#endif
