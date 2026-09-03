#include "RavememsV2Types.h"

#include <QTextStream>

using namespace RavememsV2;

namespace {

int fail(const QString &message)
{
    QTextStream(stderr) << "FAIL: " << message << Qt::endl;
    return 1;
}

} // namespace

int main()
{
    ApplicabilityContext carb;
    carb.vehicle.make = QStringLiteral("Example");
    carb.vehicle.model = QStringLiteral("Classic");
    carb.powertrain.engineFamily = QStringLiteral("A-Series");
    carb.powertrain.fuelDeliveryKind = QStringLiteral("carburetor");
    carb.control.engineManagementKind = QStringLiteral("mechanical");
    carb.control.engineControllerState = ControllerState::Absent;
    carb.powertrain.transmissionKind = QStringLiteral("manual");
    carb.equipment.insert(QStringLiteral("air_conditioning"), QStringLiteral("absent"));

    if (!carb.control.explicitlyHasNoEcu() || carb.control.hasEcu())
        return fail(QStringLiteral("carburettor/no-ECU context state failed"));
    if (!carb.control.ecuFamily.isEmpty() || !carb.control.ecuReference.isEmpty()
        || !carb.control.ecuFirmware.isEmpty())
        return fail(QStringLiteral("no-ECU context unexpectedly contains ECU identity"));

    ApplicabilityContext mpi;
    mpi.vehicle.make = QStringLiteral("Example");
    mpi.vehicle.model = QStringLiteral("MPI");
    mpi.powertrain.fuelDeliveryKind = QStringLiteral("mpi");
    mpi.control.engineManagementKind = QStringLiteral("electronic");
    mpi.control.engineControllerState = ControllerState::Present;
    mpi.control.ecuFamily = QStringLiteral("MEMS 1.9");
    mpi.control.ecuReference = QStringLiteral("MKC");
    mpi.control.ecuFirmware = QStringLiteral("AANMP002");

    if (!mpi.control.hasEcu() || mpi.control.explicitlyHasNoEcu())
        return fail(QStringLiteral("optional ECU-present context state failed"));

    Operation operation;
    operation.operationKey = QStringLiteral("OP_19_22_61");
    operation.manufacturerOperationNo = QStringLiteral("19.22.61");
    operation.titleSource = QStringLiteral("Fuel system component");
    operation.sourceLanguage = QStringLiteral("en");
    operation.applicability.append(carb);

    Phase remove;
    remove.sequenceNo = 1;
    remove.phaseKindSource = QStringLiteral("Remove");
    remove.normalizedPhaseKind = QStringLiteral("remove");
    Step remove1;
    remove1.sequenceNo = 1;
    remove1.manufacturerStepNo = QStringLiteral("1");
    remove1.instructionSource = QStringLiteral("Disconnect component");
    remove1.completenessStatus = QStringLiteral("complete");
    remove.steps.append(remove1);

    Phase refit;
    refit.sequenceNo = 2;
    refit.phaseKindSource = QStringLiteral("Refit");
    refit.normalizedPhaseKind = QStringLiteral("refit");
    Step refit1;
    refit1.sequenceNo = 1;
    refit1.manufacturerStepNo = QStringLiteral("1");
    refit1.instructionSource = QStringLiteral("Refit component");
    refit1.completenessStatus = QStringLiteral("complete");
    refit.steps.append(refit1);

    operation.phases << remove << refit;
    if (operation.phases.size() != 2
        || operation.phases.at(0).steps.at(0).manufacturerStepNo != QStringLiteral("1")
        || operation.phases.at(1).steps.at(0).manufacturerStepNo != QStringLiteral("1"))
        return fail(QStringLiteral("Remove/Refit manufacturer numbering was not preserved"));

    Specification tightening;
    tightening.specificationKey = QStringLiteral("SPEC_TIGHTEN");
    tightening.parameterSource = QStringLiteral("Fastener tightening sequence");
    tightening.conditionText = QStringLiteral("Engine cold");

    SpecificationValue pass1;
    pass1.sequenceNo = 1;
    pass1.hasNumericValue = true;
    pass1.numericValue = 20.0;
    pass1.unit = QStringLiteral("N.m");
    pass1.instructionText = QStringLiteral("Initial torque");

    SpecificationValue pass2;
    pass2.sequenceNo = 2;
    pass2.hasAngle = true;
    pass2.angleDegrees = 60.0;
    pass2.instructionText = QStringLiteral("First angular pass");

    SpecificationValue pass3;
    pass3.sequenceNo = 3;
    pass3.hasAngle = true;
    pass3.angleDegrees = 60.0;
    pass3.instructionText = QStringLiteral("Second angular pass");

    tightening.values << pass1 << pass2 << pass3;
    if (tightening.values.size() != 3
        || !tightening.values.at(0).hasNumericValue
        || tightening.values.at(0).numericValue != 20.0
        || !tightening.values.at(1).hasAngle
        || tightening.values.at(1).angleDegrees != 60.0
        || !tightening.values.at(2).hasAngle
        || tightening.values.at(2).angleDegrees != 60.0)
        return fail(QStringLiteral("sequential tightening contract failed"));

    VisualReference visual;
    visual.visualKey = QStringLiteral("VIS_TIGHTEN");
    visual.visualType = QStringLiteral("tightening_sequence");
    visual.relativePath = QStringLiteral("visuals/tightening.png");
    visual.sha256 = QString(64, QLatin1Char('a'));
    visual.fidelityStatus = QStringLiteral("verified");
    if (!visual.isUsable())
        return fail(QStringLiteral("typed verified visual contract failed"));

    QTextStream out(stdout);
    out << "RAVEMEMS_V2_CONTEXT_SELFTEST_PASS" << Qt::endl;
    out << "CARBURETOR_NO_ECU_CPP_PASS" << Qt::endl;
    out << "OPTIONAL_ECU_CPP_PASS" << Qt::endl;
    out << "STRUCTURED_OPERATION_CPP_PASS" << Qt::endl;
    out << "SEQUENTIAL_TIGHTENING_CPP_PASS" << Qt::endl;
    return 0;
}
