#include "pattern_book.h"

#include <algorithm>
#include <fstream>

using namespace std;

namespace {
vector<Position> findIgnitionPositions(const FieldPattern& pattern)
{
    Position positions[FieldConstant::MAP_WIDTH * FieldConstant::MAP_HEIGHT];

    FieldBitField checked;
    for (int x = 1; x <= 6; ++x) {
        for (int y = 1; y <= 12; ++y) {
            if (checked(x, y))
                continue;
            if (!(pattern.type(x, y) == PatternType::VAR || pattern.type(x, y) == PatternType::MUST_VAR))
                continue;
            char c = pattern.variable(x, y);
            CHECK('A' <= c && c <= 'Z');
            Position* p = pattern.fillSameVariablePositions(x, y, c, positions, &checked);
            if (p - positions >= 4) {
                std::sort(positions, p);
                return vector<Position>(positions, p);
            }
        }
    }

    CHECK(false) << "there is no 4-connected variables." << pattern.toDebugString();
    return vector<Position>();
}

} // anonymous namespace

PatternBookField::PatternBookField(const std::string& field, int ignitionColumn) :
    pattern_(field),
    ignitionColumn_(ignitionColumn),
    ignitionPositions_(findIgnitionPositions(pattern_))
{
    DCHECK(0 <= ignitionColumn && ignitionColumn <= 6);
}

PatternBookField::PatternBookField(const FieldPattern& pattern, int ignitionColumn) :
    pattern_(pattern),
    ignitionColumn_(ignitionColumn),
    ignitionPositions_(findIgnitionPositions(pattern_))
{
    DCHECK(0 <= ignitionColumn && ignitionColumn <= 6);
}

bool PatternBook::load(const string& filename)
{
    ifstream ifs(filename);
    toml::Parser parser(ifs);
    toml::Value v = parser.parse();
    if (!v.valid()) {
        LOG(ERROR) << parser.errorReason();
        return false;
    }

    return loadFromValue(std::move(v));
}

bool PatternBook::loadFromString(const string& str)
{
    istringstream ss(str);
    toml::Parser parser(ss);
    toml::Value v = parser.parse();
    if (!v.valid()) {
        LOG(ERROR) << parser.errorReason();
        return false;
    }

    return loadFromValue(v);
}

bool PatternBook::loadFromValue(const toml::Value& patterns)
{
    const toml::Array& vs = patterns.find("pattern")->as<toml::Array>();
    for (const toml::Value& v : vs) {
        string str;
        for (const auto& s : v.get<toml::Array>("field"))
            str += s.as<string>();

        int ignitionColumn = 0;
        if (const toml::Value* p = v.find("ignition")) {
            ignitionColumn = p->as<int>();
            CHECK(1 <= ignitionColumn && ignitionColumn <= 6) << ignitionColumn;
        }

        PatternBookField pbf(str, ignitionColumn);
        fields_.emplace(pbf.ignitionPositions(), pbf);

        PatternBookField mirrored(pbf.mirror());
        fields_.emplace(mirrored.ignitionPositions(), mirrored);
    }

    return true;
}

pair<PatternBook::Iterator, PatternBook::Iterator>
PatternBook::find(const vector<Position>& ignitionPositions) const
{
    return fields_.equal_range(ignitionPositions);
}

void PatternBook::iteratePossibleRensas(const CoreField& originalField,
                                        int maxIteration,
                                        const RensaDetector::TrackedPossibleRensaCallback& callback) const
{
    DCHECK_GE(maxIteration, 1);

    const CoreField::SimulationContext originalContext = CoreField::SimulationContext::fromField(originalField);

    auto detectionCallback = [this, maxIteration, &callback,
                              &originalField, &originalContext](CoreField* cf, const ColumnPuyoList& firePuyoList) {
        std::vector<Position> ignitionPositions = cf->erasingPuyoPositions(originalContext);
        if (ignitionPositions.empty())
            return;

        std::sort(ignitionPositions.begin(), ignitionPositions.end());
        std::pair<Iterator, Iterator> p = this->find(ignitionPositions);
        for (Iterator it = p.first; it != p.second; ++it) {
            const PatternBookField& pbf = it->second;
            if (pbf.ignitionColumn() == 0)
                continue;

            bool ok = true;

            bool foundFirePuyo = false;
            ColumnPuyo firePuyo;
            ColumnPuyoList keyPuyos;
            for (const ColumnPuyo& cp : firePuyoList) {
                if (foundFirePuyo) {
                    if (!keyPuyos.add(cp)) {
                        ok = false;
                        break;
                    }
                    continue;
                }

                if (cp.x == pbf.ignitionColumn()) {
                    foundFirePuyo = true;
                    firePuyo = cp;
                    continue;
                }

                keyPuyos.add(cp);
            }

            if (!ok || !foundFirePuyo)
                return;

            iteratePossibleRensasInternal(originalField, 0, *cf, firePuyo, keyPuyos, originalContext, maxIteration, callback);
        }
    };

    RensaDetectorStrategy strategy = RensaDetectorStrategy::defaultExtendStrategy();
    RensaDetector::detect(originalField, strategy, detectionCallback);
}

void PatternBook::iteratePossibleRensasInternal(const CoreField& original,
                                                int currentChains,
                                                const CoreField& field,
                                                const ColumnPuyo& firePuyo,
                                                const ColumnPuyoList& originalKeyPuyos,
                                                const CoreField::SimulationContext& fieldContext,
                                                int restIteration,
                                                const RensaDetector::TrackedPossibleRensaCallback& callback) const
{
    // Check without adding anything.
    {
        CoreField cf(field);
        CoreField::SimulationContext context(fieldContext);
        if (cf.vanishDrop(&context) > 0) {
            iteratePossibleRensasInternal(original, currentChains + 1, cf, firePuyo, originalKeyPuyos, context, restIteration, callback);
        } else {
            checkRensa(original, currentChains, firePuyo, originalKeyPuyos, fieldContext, callback);
        }
    }

    if (restIteration == 0)
        return;

    // Complement.
    std::vector<Position> ignitionPositions = field.erasingPuyoPositions(fieldContext);
    if (ignitionPositions.empty())
        return;

    std::sort(ignitionPositions.begin(), ignitionPositions.end());
    std::pair<Iterator, Iterator> p = this->find(ignitionPositions);
    for (Iterator it = p.first; it != p.second; ++it) {
        const PatternBookField& pbf = it->second;

        ColumnPuyoList cpl;
        if (!pbf.complement(field, &cpl))
            continue;
        if (cpl.size() == 0)
            continue;

        CoreField cf(field);

        bool ok = true;
        ColumnPuyoList keyPuyos(originalKeyPuyos);
        for (const ColumnPuyo& cp : cpl) {
            if (cp.x == firePuyo.x) {
                ok = false;
                break;
            }
            if (!keyPuyos.add(cp)) {
                ok = false;
                break;
            }
            if (!cf.dropPuyoOn(cp.x, cp.color)) {
                ok = false;
                break;
            }
        }

        if (!ok)
            continue;

        CoreField::SimulationContext context(fieldContext);
        int score = cf.vanishDrop(&context);
        CHECK(score > 0) << score;

        iteratePossibleRensasInternal(original, currentChains + 1, cf, firePuyo, keyPuyos, context, restIteration - 1, callback);
    }
}

void PatternBook::checkRensa(const CoreField& originalField,
                             int currentChains,
                             const ColumnPuyo& firePuyo,
                             const ColumnPuyoList& keyPuyos,
                             const CoreField::SimulationContext&,
                             const RensaDetector::TrackedPossibleRensaCallback& callback) const
{
    CoreField cf(originalField);
    CoreField::SimulationContext context = CoreField::SimulationContext::fromField(originalField);

    for (const auto& cp : keyPuyos) {
        if (!cf.dropPuyoOn(cp.x, cp.color))
            return;
    }

    // If rensa occurs after adding key puyos, this is invalid.
    if (cf.rensaWillOccurWithContext(context))
        return;

    if (!cf.dropPuyoOn(firePuyo.x, firePuyo.color))
        return;

    RensaTrackResult trackResult;
    RensaResult rensaResult = cf.simulateWithContext(&context, &trackResult);
    if (rensaResult.chains != currentChains)
        return;

    ColumnPuyoList firePuyos;
    firePuyos.add(firePuyo);
    callback(cf, rensaResult, keyPuyos, firePuyos, trackResult);
}