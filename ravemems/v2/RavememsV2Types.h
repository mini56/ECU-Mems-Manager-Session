#ifndef RAVEMEMS_V2_TYPES_H
#define RAVEMEMS_V2_TYPES_H

#include <QList>
#include <QMap>
#include <QString>

namespace RavememsV2 {

enum class ControllerState
{
    Unknown,
    Absent,
    Present
};

struct VehicleContext
{
    QString make;
    QString model;
    QString generation;
    QString bodyStyle;
    int yearFrom = 0;
    int yearTo = 0;
    QString market;
};

struct PowertrainContext
{
    QString engineFamily;
    QString engineCode;
    QString engineVariant;
    int displacementCc = 0;

    // Intentionally open-ended: carburetor, SPi, MPi, mechanical injection,
    // diesel mechanical, electric, or any future architecture documented by
    // a source manual. RAVEMEMS must not make an ECU/injection assumption.
    QString fuelDeliveryKind;
    QString inductionKind;
    QString transmissionKind;
    QString transmissionCode;
};

struct ControlContext
{
    QString engineManagementKind;
    ControllerState engineControllerState = ControllerState::Unknown;

    // Optional by design. A carburettor/mechanical vehicle may legitimately
    // have no engine ECU at all.
    QString ecuFamily;
    QString ecuReference;
    QString ecuFirmware;

    bool hasEcu() const
    {
        return engineControllerState == ControllerState::Present;
    }

    bool explicitlyHasNoEcu() const
    {
        return engineControllerState == ControllerState::Absent;
    }
};

struct ApplicabilityContext
{
    VehicleContext vehicle;
    PowertrainContext powertrain;
    ControlContext control;

    // Flexible dimensions keep the schema usable for equipment not known
    // today (air conditioning, catalyst, ABS, power steering, etc.).
    QMap<QString, QString> equipment;
    QMap<QString, QString> extraAttributes;
    QString sourceScopeText;
};

struct Provenance
{
    QString documentKey;
    QString revisionKey;
    QString pageKey;
    int physicalPage = 0;
    QString sourceElementKey;
    QString sourceBBoxJson;
};

struct VisualReference
{
    QString visualKey;
    QString visualType;
    QString relativePath;
    QString sha256;
    QString captionSource;
    QString fidelityStatus;
    Provenance provenance;

    bool isUsable() const
    {
        return !visualKey.isEmpty()
            && !visualType.isEmpty()
            && !relativePath.isEmpty()
            && !sha256.isEmpty()
            && fidelityStatus == QStringLiteral("verified");
    }
};

struct Requirement
{
    QString requirementType;
    QString sourceText;
    QString partNumber;
    double quantity = 0.0;
    QString unit;
    bool beforeStart = false;
    QString figureRef;
};

struct SpecificationValue
{
    int sequenceNo = 0;
    bool hasNumericValue = false;
    double numericValue = 0.0;
    bool hasMinimum = false;
    double minimumValue = 0.0;
    bool hasMaximum = false;
    double maximumValue = 0.0;
    bool hasAngle = false;
    double angleDegrees = 0.0;
    QString valueText;
    QString unit;
    QString conditionText;
    QString instructionText;
};

struct Specification
{
    QString specificationKey;
    QString parameterSource;
    QString defaultUnit;
    QString conditionText;
    QList<SpecificationValue> values;
};

struct Step
{
    int sequenceNo = 0;
    QString manufacturerStepNo;
    QString instructionSource;
    QString conditionText;
    QString completenessStatus;
    QList<Requirement> requirements;
    QList<Specification> specifications;
    QList<VisualReference> visuals;
    Provenance provenance;
};

struct Phase
{
    int sequenceNo = 0;
    QString phaseKindSource;
    QString normalizedPhaseKind;
    QString titleSource;
    QList<Step> steps;
};

struct Notice
{
    QString noticeKind;
    QString sourceText;
    QString scopeKind;
    QString targetKey;
};

struct Operation
{
    QString operationKey;
    QString manufacturerOperationNo;
    QString titleSource;
    QString sourceLanguage;
    QList<ApplicabilityContext> applicability;
    QList<Phase> phases;
    QList<Notice> notices;
    QList<Requirement> requirements;
    QList<Specification> specifications;
    QList<VisualReference> visuals;
    Provenance provenance;
};

} // namespace RavememsV2

#endif // RAVEMEMS_V2_TYPES_H
