#include "ExpertEngine.h"
#include "ExpertKnowledgeReader.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {

QString normalized(const QString &value)
{
    return value.trimmed().toLower();
}

double clamp01(double value)
{
    return qBound(0.0, value, 1.0);
}

QString numberText(double value)
{
    return QString::number(value, 'f', 4);
}

}

ExpertEngine::ExpertEngine() = default;

void ExpertEngine::setContext(const ExpertContext &context)
{
    m_context = context;
}

ExpertContext ExpertEngine::context() const
{
    return m_context;
}

void ExpertEngine::setKnowledgeReader(ExpertKnowledgeReader *reader)
{
    m_reader = reader;
}

void ExpertEngine::clearSamples()
{
    m_samples.clear();
}

void ExpertEngine::addSample(const ExpertObservation &sample)
{
    if (!m_samples.isEmpty() && sample.timestampMs < m_samples.last().timestampMs) {
        QList<ExpertObservation> copy = m_samples;
        copy.append(sample);
        std::sort(copy.begin(), copy.end(), [](const ExpertObservation &a, const ExpertObservation &b) {
            return a.timestampMs < b.timestampMs;
        });
        m_samples = copy;
        return;
    }
    m_samples.append(sample);
}

int ExpertEngine::sampleCount() const
{
    return m_samples.size();
}

double ExpertEngine::provenanceWeight(const QString &verificationLevel)
{
    const QString level = normalized(verificationLevel);
    if (level == QStringLiteral("verifie_constructeur"))
        return 1.00;
    if (level == QStringLiteral("decoded_by_project"))
        return 0.95;
    if (level == QStringLiteral("recoupee"))
        return 0.85;
    if (level == QStringLiteral("source_externe"))
        return 0.60;
    if (level == QStringLiteral("plausible"))
        return 0.45;
    if (level == QStringLiteral("non_verifie"))
        return 0.30;
    if (level == QStringLiteral("conflit_a_verifier"))
        return 0.0;
    return 0.25;
}

bool ExpertEngine::verificationAllowsStrongConclusion(const QString &verificationLevel)
{
    const QString level = normalized(verificationLevel);
    return level == QStringLiteral("verifie_constructeur")
        || level == QStringLiteral("decoded_by_project")
        || level == QStringLiteral("recoupee");
}

QHash<QString, ExpertSeriesStats> ExpertEngine::calculateStatistics() const
{
    struct SeriesPoint {
        qint64 timestampMs;
        double value;
    };

    QHash<QString, QList<SeriesPoint>> series;
    for (const ExpertObservation &sample : m_samples) {
        for (auto it = sample.values.constBegin(); it != sample.values.constEnd(); ++it) {
            if (!std::isfinite(it.value()))
                continue;
            series[it.key()].append({sample.timestampMs, it.value()});
        }
    }

    QHash<QString, ExpertSeriesStats> result;
    for (auto it = series.constBegin(); it != series.constEnd(); ++it) {
        const QList<SeriesPoint> &points = it.value();
        if (points.isEmpty())
            continue;

        ExpertSeriesStats stats;
        stats.count = points.size();
        stats.first = points.first().value;
        stats.last = points.last().value;
        stats.minimum = points.first().value;
        stats.maximum = points.first().value;

        double sum = 0.0;
        for (const SeriesPoint &point : points) {
            sum += point.value;
            stats.minimum = qMin(stats.minimum, point.value);
            stats.maximum = qMax(stats.maximum, point.value);
        }
        stats.mean = sum / double(points.size());
        stats.range = stats.maximum - stats.minimum;

        double varianceSum = 0.0;
        for (const SeriesPoint &point : points) {
            const double delta = point.value - stats.mean;
            varianceSum += delta * delta;
        }
        stats.standardDeviation = std::sqrt(varianceSum / double(points.size()));

        if (points.size() >= 2) {
            const double origin = double(points.first().timestampMs) / 1000.0;
            double sumX = 0.0;
            double sumY = 0.0;
            double sumXX = 0.0;
            double sumXY = 0.0;
            for (const SeriesPoint &point : points) {
                const double x = double(point.timestampMs) / 1000.0 - origin;
                const double y = point.value;
                sumX += x;
                sumY += y;
                sumXX += x * x;
                sumXY += x * y;
            }
            const double n = double(points.size());
            const double denominator = n * sumXX - sumX * sumX;
            if (std::fabs(denominator) > 1e-12)
                stats.slopePerSecond = (n * sumXY - sumX * sumY) / denominator;
        }

        result.insert(it.key(), stats);
    }
    return result;
}

bool ExpertEngine::metricValue(const QHash<QString, ExpertSeriesStats> &statistics,
                               const QString &parameter,
                               const QString &metric,
                               double *value) const
{
    if (!value || !statistics.contains(parameter))
        return false;

    const ExpertSeriesStats &stats = statistics.value(parameter);
    const QString key = normalized(metric);
    if (key.isEmpty() || key == QStringLiteral("last"))
        *value = stats.last;
    else if (key == QStringLiteral("first"))
        *value = stats.first;
    else if (key == QStringLiteral("mean"))
        *value = stats.mean;
    else if (key == QStringLiteral("min") || key == QStringLiteral("minimum"))
        *value = stats.minimum;
    else if (key == QStringLiteral("max") || key == QStringLiteral("maximum"))
        *value = stats.maximum;
    else if (key == QStringLiteral("range"))
        *value = stats.range;
    else if (key == QStringLiteral("stddev") || key == QStringLiteral("standard_deviation"))
        *value = stats.standardDeviation;
    else if (key == QStringLiteral("slope") || key == QStringLiteral("slope_per_second"))
        *value = stats.slopePerSecond;
    else if (key == QStringLiteral("count"))
        *value = double(stats.count);
    else
        return false;
    return true;
}

bool ExpertEngine::conditionMatches(const ExpertRuleCondition &condition,
                                    const QHash<QString, ExpertSeriesStats> &statistics,
                                    QString *evidence) const
{
    double left = 0.0;
    if (!metricValue(statistics, condition.parameter, condition.metric, &left))
        return false;

    const QString comparator = normalized(condition.comparator);
    double right = 0.0;
    const bool hasRightParameter = !condition.rightParameter.trimmed().isEmpty();
    if (hasRightParameter) {
        if (!metricValue(statistics,
                         condition.rightParameter,
                         condition.rightMetric.isEmpty() ? condition.metric : condition.rightMetric,
                         &right))
            return false;
        right = right * condition.rightMultiplier + condition.rightOffset;
    }

    bool match = false;
    if (comparator == QStringLiteral("gt"))
        match = left > (hasRightParameter ? right : condition.thresholdLow);
    else if (comparator == QStringLiteral("ge"))
        match = left >= (hasRightParameter ? right : condition.thresholdLow);
    else if (comparator == QStringLiteral("lt"))
        match = left < (hasRightParameter ? right : condition.thresholdLow);
    else if (comparator == QStringLiteral("le"))
        match = left <= (hasRightParameter ? right : condition.thresholdLow);
    else if (comparator == QStringLiteral("between"))
        match = left >= condition.thresholdLow && left <= condition.thresholdHigh;
    else if (comparator == QStringLiteral("outside"))
        match = left < condition.thresholdLow || left > condition.thresholdHigh;
    else if (comparator == QStringLiteral("absdiff_gt") && hasRightParameter)
        match = std::fabs(left - right) > condition.thresholdLow;
    else if (comparator == QStringLiteral("absdiff_ge") && hasRightParameter)
        match = std::fabs(left - right) >= condition.thresholdLow;
    else if (comparator == QStringLiteral("absdiff_lt") && hasRightParameter)
        match = std::fabs(left - right) < condition.thresholdLow;
    else if (comparator == QStringLiteral("absdiff_le") && hasRightParameter)
        match = std::fabs(left - right) <= condition.thresholdLow;

    if (match && evidence) {
        QString text = QStringLiteral("%1.%2=%3")
                           .arg(condition.parameter,
                                condition.metric.isEmpty() ? QStringLiteral("last") : condition.metric,
                                numberText(left));
        if (hasRightParameter) {
            text += QStringLiteral(" ; %1.%2 ajusté=%3")
                        .arg(condition.rightParameter,
                             condition.rightMetric.isEmpty() ? condition.metric : condition.rightMetric,
                             numberText(right));
        } else if (comparator == QStringLiteral("between") || comparator == QStringLiteral("outside")) {
            text += QStringLiteral(" ; bornes=%1..%2")
                        .arg(numberText(condition.thresholdLow), numberText(condition.thresholdHigh));
        } else {
            text += QStringLiteral(" ; seuil=%1").arg(numberText(condition.thresholdLow));
        }
        if (!condition.notes.isEmpty())
            text += QStringLiteral(" ; ") + condition.notes;
        *evidence = text;
    }
    return match;
}

ExpertHypothesis ExpertEngine::evaluateRule(const ExpertRule &rule,
                                            const QHash<QString, ExpertSeriesStats> &statistics,
                                            bool *matched) const
{
    ExpertHypothesis hypothesis;
    hypothesis.ruleKey = rule.ruleKey;
    hypothesis.title = rule.title;
    hypothesis.category = rule.category;
    hypothesis.severity = rule.severity;
    hypothesis.verificationLevel = rule.verificationLevel;
    hypothesis.recommendations = rule.recommendations;

    if (matched)
        *matched = false;

    if (provenanceWeight(rule.verificationLevel) <= 0.0)
        return hypothesis;
    if (m_samples.size() < qMax(1, rule.minimumSamples))
        return hypothesis;
    if (rule.conditions.isEmpty())
        return hypothesis;

    double totalWeight = 0.0;
    double matchedWeight = 0.0;
    bool allRequired = true;

    for (const ExpertRuleCondition &condition : rule.conditions) {
        const double weight = qMax(0.0, condition.weight);
        totalWeight += weight;
        QString evidence;
        const bool conditionMatched = conditionMatches(condition, statistics, &evidence);
        if (conditionMatched) {
            matchedWeight += weight;
            if (!evidence.isEmpty())
                hypothesis.evidence.append(evidence);
        } else if (condition.required) {
            allRequired = false;
        }
    }

    if (!allRequired || totalWeight <= 0.0)
        return hypothesis;

    const double coverage = matchedWeight / totalWeight;
    const double provenance = provenanceWeight(rule.verificationLevel);
    hypothesis.confidence = clamp01(rule.baseConfidence * coverage * provenance);
    hypothesis.strongConclusionAllowed = verificationAllowsStrongConclusion(rule.verificationLevel)
        && hypothesis.confidence >= 0.70;

    if (!rule.notes.isEmpty())
        hypothesis.evidence.append(QStringLiteral("Règle: ") + rule.notes);

    if (matched)
        *matched = true;
    return hypothesis;
}

ExpertAnalysisResult ExpertEngine::analyze() const
{
    ExpertAnalysisResult result;
    result.context = m_context;
    result.sampleCount = m_samples.size();
    result.statistics = calculateStatistics();

    if (!m_reader || !m_reader->isOpen())
        return result;

    result.contextFacts = m_reader->facts(m_context);
    const QList<ExpertRule> rules = m_reader->rules(m_context);
    for (const ExpertRule &rule : rules) {
        bool matched = false;
        ExpertHypothesis hypothesis = evaluateRule(rule, result.statistics, &matched);
        if (matched)
            result.hypotheses.append(hypothesis);
    }

    std::sort(result.hypotheses.begin(), result.hypotheses.end(),
              [](const ExpertHypothesis &a, const ExpertHypothesis &b) {
        if (std::fabs(a.confidence - b.confidence) > 1e-9)
            return a.confidence > b.confidence;
        if (a.severity != b.severity)
            return a.severity > b.severity;
        return a.ruleKey < b.ruleKey;
    });

    return result;
}
