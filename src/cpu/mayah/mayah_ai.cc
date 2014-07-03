#include "mayah_ai.h"

#include <gflags/gflags.h>
#include <sstream>

#include "core/algorithm/plan.h"
#include "core/algorithm/puyo_possibility.h"
#include "core/client/connector/drop_decision.h"
#include "core/frame_data.h"

#include "feature_parameter.h"
#include "evaluator.h"
#include "gazer.h"
#include "util.h"

DECLARE_string(feature);

using namespace std;

MayahAI::MayahAI() :
    AI("mayah")
{
    featureParameter_.reset(new FeatureParameter(FLAGS_feature));
    LOG(INFO) << featureParameter_->toString();
}

MayahAI::~MayahAI()
{
}

void MayahAI::gameWillBegin(const FrameData& frameData)
{
    gazer_.initializeWith(frameData.id);
}

void MayahAI::gameHasEnded(const FrameData&)
{
}

DropDecision MayahAI::think(int frameId, const PlainField& plainField, const KumipuyoSeq& kumipuyoSeq)
{
    CoreField f(plainField);
    double beginTime = now();
    Plan plan = thinkPlan(frameId, f, kumipuyoSeq, false);
    double endTime = now();
    std::string message = makeMessageFrom(frameId, f, kumipuyoSeq, false, plan, endTime - beginTime);
    if (plan.decisions().empty())
        return DropDecision(Decision(3, 0), message);
    return DropDecision(plan.decisions().front(), message);
}

DropDecision MayahAI::thinkFast(int frameId, const PlainField& plainField, const KumipuyoSeq& kumipuyoSeq)
{
    CoreField f(plainField);
    double beginTime = now();
    Plan plan = thinkPlan(frameId, f, kumipuyoSeq, true);
    double endTime = now();
    std::string message = makeMessageFrom(frameId, f, kumipuyoSeq, true, plan, endTime - beginTime);
    if (plan.decisions().empty())
        return DropDecision(Decision(3, 0), message);
    return DropDecision(plan.decisions().front(), message);
}

Plan MayahAI::thinkPlan(int frameId, const CoreField& field, const KumipuyoSeq& kumipuyoSeq, bool fast) const
{
    LOG(INFO) << "\n" << field.toDebugString() << "\n" << kumipuyoSeq.toString();

#if 0
    int currentMaxRensa = 0;
    {
        RefPlan refPlan(field, vector<Decision>(), RensaResult(), 0, 0);
        CollectedFeature cf = Evaluator(*featureParameter_).evalWithCollectingFeature(refPlan, field, frameId, fast, gazer_);
        currentMaxRensa = cf.feature(MAX_CHAINS).empty() ? 0 : cf.feature(MAX_CHAINS).front();
    }
#endif

    double bestScore = -100000000.0;
    Plan bestPlan;
    Plan::iterateAvailablePlans(field, kumipuyoSeq, 2,
                                [this, frameId, fast, &field, &bestScore, &bestPlan](const RefPlan& plan) {
        double score = Evaluator(*featureParameter_).eval(plan, field, frameId, fast, gazer_);
        if (bestScore < score) {
            bestScore = score;
            bestPlan = plan.toPlan();
        }
    });

    return bestPlan;
}

std::string MayahAI::makeMessageFrom(int frameId, const CoreField& field, const KumipuyoSeq& kumipuyoSeq, bool fast,
                                     const Plan& plan, double thoughtTimeInSeconds) const
{
    UNUSED_VARIABLE(kumipuyoSeq);

    if (plan.decisions().empty())
        return string("give up :-(");

    RefPlan refPlan(plan.field(), plan.decisions(), plan.rensaResult(), plan.numChigiri(), plan.initiatingFrames());
    CollectedFeature cf = Evaluator(*featureParameter_).evalWithCollectingFeature(refPlan, field, frameId, fast, gazer_);

    stringstream ss;
    if (cf.feature(STRATEGY_ZENKESHI) > 0)
        ss << "ZENKESHI / ";
    if (cf.feature(STRATEGY_TAIOU) > 0)
        ss << "TAIOU / ";
    if (cf.feature(STRATEGY_LARGE_ENOUGH) > 0)
        ss << "LARGE_ENOUGH / ";
    if (cf.feature(STRATEGY_TSUBUSHI) > 0)
        ss << "TSUBUSHI / ";
    if (cf.feature(STRATEGY_SAKIUCHI) > 0)
        ss << "SAKIUCHI / ";
    if (cf.feature(STRATEGY_HOUWA) > 0)
        ss << "HOUWA / ";
    if (cf.feature(STRATEGY_SAISOKU) > 0)
        ss << "SAISOKU / ";

    ss << "SCORE = " << cf.score() << " / ";

    if (!cf.feature(MAX_CHAINS).empty()) {
        const vector<int>& vs = cf.feature(MAX_CHAINS);
        for (size_t i = 0; i < vs.size(); ++i)
            ss << "MAX CHAIN = " << vs[i] << " / ";
    }

    ss << "Gazed max score = "
       << gazer_.estimateMaxScore(frameId + refPlan.totalFrames())
       << " in " << refPlan.totalFrames() << " / ";

    ss << (thoughtTimeInSeconds * 1000) << " [ms]";

    return ss.str();
}

void MayahAI::enemyGrounded(const FrameData& frameData)
{
    gazer_.setId(frameData.id);

    // --- Check if Rensa starts.
    CoreField field(frameData.enemyPlayerFrameData().field);
    field.forceDrop();

    RensaResult rensaResult = field.simulate();

    if (rensaResult.chains > 0)
        gazer_.setOngoingRensa(OngoingRensaInfo(rensaResult, frameData.id + rensaResult.frames));
    else
        gazer_.setRensaIsOngoing(false);
}

void MayahAI::enemyNext2Appeared(const FrameData& frameData)
{
    // At the beginning of the game, kumipuyoSeq might contain EMPTY/EMPTY.
    // In that case, we need to skip.
    const KumipuyoSeq& seq = frameData.enemyPlayerFrameData().kumipuyoSeq;
    if (!isNormalColor(seq.axis(0)) || !isNormalColor(seq.child(0)))
        return;

    int currentFrameId = frameData.id;

    gazer_.setId(currentFrameId);
    gazer_.updateFeasibleRensas(frameData.enemyPlayerFrameData().field, frameData.enemyPlayerFrameData().kumipuyoSeq);
    gazer_.updatePossibleRensas(frameData.enemyPlayerFrameData().field, frameData.enemyPlayerFrameData().kumipuyoSeq);

    LOG(INFO) << "Possible rensa infos : ";
    for (auto it = gazer_.possibleRensaInfos().begin(); it != gazer_.possibleRensaInfos().end(); ++it)
        LOG(INFO) << it->toString();
    LOG(INFO) << "Feasible rensa infos : ";
    for (auto it = gazer_.feasibleRensaInfos().begin(); it != gazer_.feasibleRensaInfos().end(); ++it)
        LOG(INFO) << it->toString();
}
