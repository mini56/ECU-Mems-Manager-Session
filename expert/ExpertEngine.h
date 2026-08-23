#ifndef EXPERTENGINE_H
#define EXPERTENGINE_H

#include "ExpertTypes.h"

class ExpertKnowledgeReader;

class ExpertEngine
{
public:
    ExpertEngine();

    void setContext(const ExpertContext &context);
    ExpertContext context() const;

    void setKnowledgeReader(ExpertKnowledgeReader *reader);
    void clearSamples();
    void addSample(const ExpertObservation &sample);
    int sampleCount() const;

    ExpertAnalysisResult analyze() const;

    static double provenanceWeight(const QString &verificationLevel);
    static bool verificationAllowsStrongConclusion(const QString &verificationLevel);

private:
    QHash<QString, ExpertSeriesStats> calculateStatistics() const;
    bool metricValue(const QHash<QString, ExpertSeriesStats> &statistics,
                     const QString &parameter,
                     const QString &metric,
                     double *value) const;
    bool conditionMatches(const ExpertRuleCondition &condition,
                          const QHash<QString, ExpertSeriesStats> &statistics,
                          QString *evidence) const;
    ExpertHypothesis evaluateRule(const ExpertRule &rule,
                                  const QHash<QString, ExpertSeriesStats> &statistics,
                                  bool *matched) const;

private:
    ExpertContext m_context;
    ExpertKnowledgeReader *m_reader = nullptr;
    QList<ExpertObservation> m_samples;
};

#endif // EXPERTENGINE_H
