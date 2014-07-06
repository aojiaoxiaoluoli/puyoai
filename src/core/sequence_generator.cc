#include "core/sequence_generator.h"

#include <algorithm>
#include <random>
#include <vector>

using namespace std;

KumipuyoSeq generateSequence()
{
    random_device rd;
    mt19937 mt(rd());

    return generateSequenceWithMt19937(mt);
}

KumipuyoSeq generateSequenceWithSeed(int seed)
{
    mt19937 mt(seed);
    return generateSequenceWithMt19937(mt);
}

KumipuyoSeq generateSequenceWithMt19937(mt19937 mt)
{
    // AC puyo2 algorithm
    // make 64 x 4 sequece -> shuffle
    // if the first 3 hands contains 4 colors, make them 3 colors.
    //
    // In our algorithm, make it easy a little bit.
    // first, we shuffle [0, 64*3), then, shuffle [6, 64*4).
    vector<PuyoColor> colors(256);
    for (int i = 0; i < NUM_NORMAL_PUYO_COLORS; ++i) {
        PuyoColor c = NORMAL_PUYO_COLORS[i];
        for (int j = 0; j < 64; ++j) {
            colors[i * 64 + j] = c;
        }
    }

    // TODO(mayah): Maybe srand() is used somewhere? We need to deprecate it.
    shuffle(colors.begin(), colors.begin() + 64 * 3, mt);
    shuffle(colors.begin() + 6, colors.end(), mt);

    vector<Kumipuyo> seq(128);
    for (int i = 0; i < 128; ++i) {
        seq[i].axis = colors[2 * i];
        seq[i].child = colors[2 * i + 1];
    }

    return KumipuyoSeq(seq);
}
