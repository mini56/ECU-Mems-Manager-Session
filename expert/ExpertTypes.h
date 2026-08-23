#ifndef EXPERTTYPES_H
#define EXPERTTYPES_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

struct ExpertContext
{
    QString family;
    QString firmware;
};

struct ExpertObservation
{
    qint64 timestampMs = 0;
    QHash<QString, double> values;
};

struct ExpertSeriesStats
{
    int count = 0;
    double first = 0.0;
    double last = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    double mean = 0.0;
    double standardDeviation = 0.0;
    double range = 0.0;
    double slopePerSecond = 0.0;
};

struct ExpertFact
{
    QString sourceKey;
    QString factKey;
    QString family;
    QString firmware;
    QString topic;
    QString statement;
    QString verificationLevel;
    QString notes;
};

struct ExpertRuleCondition
{
    QString parameter;
    QString metric;
    QString comparator;
    QString rightParameter;
    QString rightMetric;
    double thresholdLow = 0.0;
    double thresholdHigh = 0.0;
    double rightMultiplier = 1.0;
    double rightOffset = 0.0;
    bool required = true;
    double weight = 1.0;
    QString notes;
};

struct ExpertRule
{
    QString ruleKey;
    QString family;
    QString firmware;
    QString title;
    QString category;
    int severity = 0;
    double baseConfidence = 0.5;
    int minimumSamples = 1;
    QString verificationLevel;
    QString notes;
    QList<ExpertRuleCondition> conditions;
    QStringList recommendations;
};

struct ExpertHypothesis
{
    QString ruleKey;
    QString title;
    QString category;
    int severity = 0;
    double confidence = 0.0;
    QString verificationLevel;
    bool strongConclusionAllowed = false;
    QStringList evidence;
    QStringList recommendations;
};

struct ExpertAnalysisResult
{
    ExpertContext context;
    int sampleCount = 0;
    QHash<QString, ExpertSeriesStats> statistics;
    QList<ExpertFact> contextFacts;
    QList<ExpertHypothesis> hypotheses;
};

#endif // EXPERTTYPES_H
