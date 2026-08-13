#include "summarytab.h"
#include <QHeaderView>
#include <QIcon>
#include "i18n.h"
#define tr I18n::text


SummaryTab::SummaryTab(QWidget *parent) : QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  m_rowCount = 0;
  m_rowsPerTable = 19;

  m_table0 = new QTableWidget(0, 4, this);
  m_table1 = new QTableWidget(0, 4, this);
  m_table2 = new QTableWidget(0, 4, this);

  QTableWidget *tables[3] = { m_table0, m_table1, m_table2 };
  for (int i = 0; i < 3; i++)
  {
    tables[i]->setHorizontalHeaderLabels(QStringList()
        << I18n::text(6600) /* EN: Parameter */ << "" << I18n::text(6601) /* EN: ECU received */ << I18n::text(6602) /* EN: Interpreted */);
    tables[i]->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tables[i]->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    tables[i]->setColumnWidth(1, 24);
    tables[i]->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    tables[i]->setColumnWidth(2, 105);
    tables[i]->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    tables[i]->setColumnWidth(3, 105);
    tables[i]->verticalHeader()->setVisible(false);
    tables[i]->setEditTriggers(QTableWidget::NoEditTriggers);
    tables[i]->setSelectionMode(QTableWidget::NoSelection);
    tables[i]->setAlternatingRowColors(true);
    layout->addWidget(tables[i]);
  }

  m_rowEngineRpm             = addRow(I18n::text(6603) /* EN: Engine speed (rpm) */);
  m_rowCoolantTemp           = addRow(I18n::text(6604) /* EN: Coolant temperature (°C) */);
  m_rowAmbientTemp           = addRow(I18n::text(6605) /* EN: Ambient temperature (°C) */);
  m_rowIntakeAirTemp         = addRow(I18n::text(6606) /* EN: Intake air temperature (°C) */);
  m_rowFuelTemp              = addRow(I18n::text(6607) /* EN: Fuel temperature (°C) */);
  m_rowMapKpa                = addRow(I18n::text(6608) /* EN: Manifold pressure (kPa) */);
  m_rowBatteryVoltage        = addRow(I18n::text(6609) /* EN: Battery voltage (V) */);
  m_rowThrottlePot           = addRow(I18n::text(6610) /* EN: Throttle position (%) */);
  m_rowIdleSwitch            = addRow(I18n::text(6611) /* EN: Idle switch */);
  m_rowUk1                   = addRow(I18n::text(6612) /* EN: Undocumented 1 */);
  m_rowParkNeutralSwitch     = addRow(I18n::text(6613) /* EN: Park/neutral switch */);
  m_rowFaultCodes            = addRow(I18n::text(6614) /* EN: Fault codes (raw) */);
  m_rowIdleSetPoint          = addRow(I18n::text(6615) /* EN: Idle set point */);
  m_rowIdleHot               = addRow(I18n::text(6616) /* EN: Hot idle */);
  m_rowUk2                   = addRow(I18n::text(6617) /* EN: Undocumented 2 */);
  m_rowIacPosition           = addRow(I18n::text(6618) /* EN: Stepper motor position (%) */);
  m_rowIdleError             = addRow(I18n::text(6619) /* EN: Idle error */);
  m_rowIgnitionAdvanceOffset = addRow(I18n::text(6620) /* EN: Ignition advance offset */);
  m_rowIgnitionAdvance       = addRow(I18n::text(6621) /* EN: Ignition advance (°) */);
  m_rowCoilTime              = addRow(I18n::text(6622) /* EN: Coil time (ms) */);
  m_rowUk3                   = addRow(I18n::text(6623) /* EN: Undocumented 3 */);
  m_rowUk4                   = addRow(I18n::text(6624) /* EN: Undocumented 4 */);
  m_rowUk5                   = addRow(I18n::text(6625) /* EN: Undocumented 5 */);
  m_rowIgnitionSwitch        = addRow(I18n::text(6626) /* EN: Ignition switch */);
  m_rowThrottleAngle         = addRow(I18n::text(6627) /* EN: Throttle angle (°) */);
  m_rowUk6                   = addRow(I18n::text(6628) /* EN: Undocumented 6 */);
  m_rowAirFuelRatio          = addRow(I18n::text(6629) /* EN: Air/fuel ratio */);
  m_rowDtc2                  = addRow(I18n::text(6630) /* EN: DTC 2 */);
  m_rowLambdaVoltage         = addRow(I18n::text(6631) /* EN: Lambda sensor voltage (mV) */);
  m_rowLambdaFrequency       = addRow(I18n::text(6632) /* EN: Lambda frequency */);
  m_rowLambdaDutycycle       = addRow(I18n::text(6633) /* EN: Lambda duty cycle (%) */);
  m_rowLambdaStatus          = addRow(I18n::text(6634) /* EN: Lambda status */);
  m_rowClosedLoop            = addRow(I18n::text(6635) /* EN: Closed loop */);
  m_rowLongTermFuelTrim      = addRow(I18n::text(6636) /* EN: Long-term fuel trim (%) */);
  m_rowShortTermFuelTrim     = addRow(I18n::text(6637) /* EN: Short-term fuel trim (%) */);
  m_rowCarbonCanisterDutycycle = addRow(I18n::text(6638) /* EN: Canister purge duty cycle (%) */);
  m_rowDtc3                  = addRow(I18n::text(6639) /* EN: DTC 3 */);
  m_rowIdleBasePos           = addRow(I18n::text(6640) /* EN: Idle base position */);
  m_rowUk7                   = addRow(I18n::text(6641) /* EN: Undocumented 7 */);
  m_rowDtc4                  = addRow(I18n::text(6642) /* EN: DTC 4 */);
  m_rowIgnitionAdvance2      = addRow(I18n::text(6643) /* EN: Ignition advance 2 */);
  m_rowIdleSpeedOffset       = addRow(I18n::text(6644) /* EN: Idle speed offset */);
  m_rowIdleErrorHotCorrected = addRow(I18n::text(6645) /* EN: Hot idle error (corrected) */);
  m_rowUk10                  = addRow(I18n::text(6646) /* EN: Raw 7D14-15 frame */);
  m_rowDtc5                  = addRow(I18n::text(6647) /* EN: DTC 5 */);
  m_rowUk11                  = addRow(I18n::text(6648) /* EN: Undocumented 11 */);
  m_rowUk12                  = addRow(I18n::text(6649) /* EN: Undocumented 12 */);
  m_rowUk13                  = addRow(I18n::text(6650) /* EN: Undocumented 13 */);
  m_rowUk14                  = addRow(I18n::text(6651) /* EN: Undocumented 14 */);
  m_rowUk15                  = addRow(I18n::text(6652) /* EN: Undocumented 15 */);
  m_rowUk16                  = addRow(I18n::text(6653) /* EN: Undocumented 16 */);
  m_rowUk1A                  = addRow(I18n::text(6654) /* EN: Undocumented 1A */);
  m_rowUk1B                  = addRow(I18n::text(6655) /* EN: Undocumented 1B */);
  m_rowUk1C                  = addRow(I18n::text(6656) /* EN: Undocumented 1C */);
  m_rowDtc0                  = addRow(I18n::text(6657) /* EN: DTC 0 */);
  m_rowDtc1                  = addRow(I18n::text(6658) /* EN: DTC 1 */);

  // Info-bulles explicatives, reprises des textes déjà présents ailleurs
  // dans le logiciel (onglets Aperçu et Réglages). Les lignes concernées
  // sont repérables par l'icône en forme de bulle devant leur nom.
  setTooltip(m_rowCoolantTemp,
    I18n::text(6687) /* EN: Displays coolant temperature measured by the ECU. If the sensor circuit is open, a default value of about 60°C is displayed. During warm-up the value should rise progressively from ambient temperature to about 90°C. A faulty sensor can cause difficult starting, excessively high idle, high fuel consumption and continuously running fans. */);
  setTooltip(m_rowAmbientTemp,
    I18n::text(6688) /* EN: Displays ambient-air temperature measured by the ECU (if equipped). If the sensor circuit is open, a fixed default value is displayed. The ECU uses this sensor to measure engine-bay temperature, generally to control an additional fan. On MEMS ECUs that do not support this sensor, N/S is displayed. */);
  setTooltip(m_rowIntakeAirTemp,
    I18n::text(6689) /* EN: Displays intake-air temperature measured by the ECU (if equipped). If the sensor circuit is open, a fixed default value is displayed. The ECU uses this temperature to retard ignition to prevent knock and to adjust hot mixture. */);
  setTooltip(m_rowMapKpa,
    I18n::text(6690) /* EN: Displays pressure measured by the MEMS internal air-pressure sensor. It should show atmospheric pressure of about 100 kPa with the engine stopped and a lower value of 25-40 kPa at idle. Very high values may indicate an internal MEMS sensor problem or, more commonly, a blocked or disconnected vacuum hose. */);
  setTooltip(m_rowBatteryVoltage,
    I18n::text(6691) /* EN: Displays vehicle supply voltage measured internally by the ECU. Large deviations can cause starting difficulties and idle CO errors. */);
  setTooltip(m_rowThrottlePot,
    I18n::text(6692) /* EN: Displays throttle position obtained by the MEMS ECU from the throttle potentiometer. The value should move from low to high as the accelerator pedal is pressed. */);
  setTooltip(m_rowIdleSwitch,
    I18n::text(6693) /* EN: Displays throttle-switch status (if fitted). If the switch indicates 'ON' while the throttle is closed, the vehicle will not idle correctly and the closed-throttle position may need to be reset (fully press/release the accelerator 5 times within 10 seconds after switching the ignition on, then wait 20 seconds). */);
  setTooltip(m_rowParkNeutralSwitch,
    I18n::text(6694) /* EN: Displays park/neutral switch status measured by the MEMS ECU. This switch improves idle control on automatic or CVT vehicles. It is not used on manual-transmission vehicles. */);
  setTooltip(m_rowIdleSpeedOffset,
    I18n::text(6695) /* EN: If an idle service offset is configured in this ECU, the difference from normal idle is displayed here. Normally only two values are possible: 0 rpm or 49 rpm. */);
  setTooltip(m_rowIacPosition,
    I18n::text(6696) /* EN: This is the number of stepper-motor steps from fully closed (0) learned by the ECU as the correct position to maintain target idle with the engine fully warm. A value outside 10-50 steps may indicate a fault or incorrect adjustment. */);
  setTooltip(m_rowIdleError,
    I18n::text(6697) /* EN: This is the current difference between the MEMS ECU target idle speed and actual engine speed. A value above 100 rpm indicates that the ECU is not controlling idle correctly and may indicate a fault. */);
  setTooltip(m_rowIdleErrorHotCorrected,
    I18n::text(6698) /* EN: Decodes field 7D14-15: the raw 16-bit value is centred on 32768, then the hot-idle position correction configured in the Settings tab is applied. Formula: (raw value - 32768) + hot-idle correction. The correction is not constant; it follows the user's current setting. */);
  setTooltip(m_rowIgnitionAdvanceOffset,
    I18n::text(6699) /* EN: Displays the service ignition-advance offset currently used by the MEMS ECU. This special setting is intended for countries using low-octane fuel and can be configured in the Settings tab. */);
  setTooltip(m_rowCoilTime,
    I18n::text(6700) /* EN: This is ignition-coil charge time to nominal current, measured by the MEMS ECU. At about 14 V battery voltage it should be around 2-3 ms. A high value may indicate a problem in the coil primary circuit. */);
  setTooltip(m_rowThrottleAngle,
    I18n::text(6701) /* EN: Displays throttle position obtained by the MEMS ECU from the throttle potentiometer. The value should move from low to high as the accelerator pedal is pressed. */);
  setTooltip(m_rowLambdaVoltage,
    I18n::text(6702) /* EN: Displays oxygen-sensor voltage read by the MEMS ECU. With the engine fully warm under normal idle or driving conditions, this voltage rapidly switches between 0.0-0.2 V and 0.7-1.0 V. */);
  setTooltip(m_rowLambdaStatus,
    I18n::text(6703) /* EN: Displays the MEMS internal diagnostic status for the oxygen sensor and its wiring. ON means no fault; OFF indicates a possible problem. */);
  setTooltip(m_rowClosedLoop,
    I18n::text(6704) /* EN: Shows whether fuelling is controlled by oxygen-sensor feedback. With the vehicle fully warm, loop status should indicate closed loop under most driving and idle conditions. */);
  setTooltip(m_rowLongTermFuelTrim,
    I18n::text(6705) /* EN: Displays current feedback fuel correction as a percentage of the mapped value. High values (e.g. 120%) indicate compensation for a mixture that is too lean; low values (e.g. 80%) indicate compensation for a mixture that is too rich. */);
  setTooltip(m_rowShortTermFuelTrim,
    I18n::text(6706) /* EN: Short-term fuel trim is active only in closed loop. Values up to 10% are normal and occasionally up to 15%. Values above 15% are abnormal. */);
  setTooltip(m_rowIgnitionAdvance2,
    I18n::text(6707) /* EN: MEMS ECU function intended to compensate for certain conditions over the vehicle's life (low-octane fuel, engine wear) by slightly advancing ignition at idle. */);
}

int SummaryTab::addRow(const QString &name)
{
  const int globalRow = m_rowCount++;
  int tableIdx = globalRow / m_rowsPerTable;
  if (tableIdx > 2) tableIdx = 2;
  const int localRow = globalRow % m_rowsPerTable;
  QTableWidget *table = (tableIdx == 0) ? m_table0 : (tableIdx == 1) ? m_table1 : m_table2;

  table->insertRow(localRow);
  table->setItem(localRow, 0, new QTableWidgetItem(name));
  table->setItem(localRow, 1, new QTableWidgetItem(""));
  table->setItem(localRow, 2, new QTableWidgetItem("--"));
  table->setItem(localRow, 3, new QTableWidgetItem("--"));
  return globalRow;
}

void SummaryTab::setTooltip(int globalRow, const QString &text)
{
  if (globalRow < 0) return;
  int tableIdx = globalRow / m_rowsPerTable;
  if (tableIdx > 2) tableIdx = 2;
  int localRow = globalRow % m_rowsPerTable;
  QTableWidget *table = (tableIdx == 0) ? m_table0 : (tableIdx == 1) ? m_table1 : m_table2;
  if (localRow >= 0 && localRow < table->rowCount())
  {
    table->item(localRow, 0)->setToolTip(text);
    table->item(localRow, 1)->setToolTip(text);
    table->item(localRow, 2)->setToolTip(text);
    table->item(localRow, 3)->setToolTip(text);
    // La bulle est dans sa propre colonne pour séparer clairement paramètre / aide / valeur.
    table->item(localRow, 1)->setText("");
    table->item(localRow, 1)->setIcon(QIcon(":/icons/helpbubble.png"));
  }
}

void SummaryTab::setValue(int globalRow, const QString &rawText, const QString &interpretedText)
{
  if (globalRow < 0) return;
  int tableIdx = globalRow / m_rowsPerTable;
  if (tableIdx > 2) tableIdx = 2;
  int localRow = globalRow % m_rowsPerTable;
  QTableWidget *table = (tableIdx == 0) ? m_table0 : (tableIdx == 1) ? m_table1 : m_table2;
  if (localRow >= 0 && localRow < table->rowCount())
  {
    table->item(localRow, 2)->setText(rawText);
    table->item(localRow, 3)->setText(interpretedText);
  }
}

void SummaryTab::updateData(mems_data *data)
{
  if (!data) return;

  // Colonne 1 : valeur reçue directement de l'ECU.
  // Colonne 2 : valeur interprétée selon les mêmes conversions que l'onglet
  // « Toutes les données ».

  setValue(m_rowEngineRpm,
           QString::number(data->engine_rpm),
           QString::number(data->engine_rpm) + " RPM");

  setValue(m_rowCoolantTemp,
           QString::number(data->coolant_temp),
           QString::number(data->coolant_temp - 55) + " °C");
  setValue(m_rowAmbientTemp,
           QString::number(data->ambient_temp),
           QString::number(data->ambient_temp - 55) + " °C");
  setValue(m_rowIntakeAirTemp,
           QString::number(data->intake_air_temp),
           QString::number(data->intake_air_temp - 55) + " °C");
  setValue(m_rowFuelTemp,
           QString::number(data->fuel_temp),
           QString::number(data->fuel_temp - 55) + " °C");

  setValue(m_rowMapKpa,
           QString::number(data->map_kpa),
           QString::number(data->map_kpa) + " kPa");

  setValue(m_rowBatteryVoltage,
           QString::number(data->battery_voltage),
           QString::number(data->battery_voltage / 10.0, 'f', 1) + " V");

  setValue(m_rowThrottlePot,
           QString::number(data->throttle_pot),
           QString::number(data->throttle_pot * 0.02, 'f', 2) + " V");

  setValue(m_rowIdleSwitch,
           QString::number(data->idle_switch),
           data->idle_switch ? I18n::text(6659) /* EN: Off */ : I18n::text(6660) /* EN: On */);

  setValue(m_rowUk1, QString::number(data->uk1),
           data->uk1 ? I18n::text(6661) /* EN: On */ : I18n::text(6662) /* EN: Off */);

  setValue(m_rowParkNeutralSwitch,
           QString::number(data->park_neutral_switch),
           data->park_neutral_switch ? I18n::text(6663) /* EN: On */ : I18n::text(6664) /* EN: Off */);

  setValue(m_rowFaultCodes,
           QString::number(data->fault_codes),
           QString::number(data->fault_codes));

  setValue(m_rowIdleSetPoint,
           QString::number(data->idle_set_point),
           QString::number(data->idle_set_point) + " RPM");

  setValue(m_rowIdleHot,
           QString::number(data->idle_hot),
           QString::number(data->idle_hot - 35));

  setValue(m_rowUk2, QString::number(data->uk2), QString::number(data->uk2));

  setValue(m_rowIacPosition,
           QString::number(data->iac_position),
           QString::number(qRound(data->iac_position / 1.8)) + " %");

  setValue(m_rowIdleError,
           QString::number(data->idle_error),
           QString::number(data->idle_error) + " RPM");

  setValue(m_rowIgnitionAdvanceOffset,
           QString::number(data->ignition_advance_offset),
           QString::number(data->ignition_advance_offset));

  setValue(m_rowIgnitionAdvance,
           QString::number(data->ignition_advance),
           QString::number((data->ignition_advance / 2.0) - 24.0, 'f', 1) + " °");

  setValue(m_rowCoilTime,
           QString::number(data->coil_time),
           QString::number(data->coil_time * 0.002, 'f', 3) + " ms");

  setValue(m_rowUk3, QString::number(data->uk3),
           data->uk3 ? I18n::text(6665) /* EN: signal */ : I18n::text(6666) /* EN: NO SIGNAL */);
  setValue(m_rowUk4, QString::number(data->uk4), QString::number(data->uk4));
  setValue(m_rowUk5, QString::number(data->uk5), QString::number(data->uk5));

  setValue(m_rowIgnitionSwitch,
           QString::number(data->ignition_switch),
           data->ignition_switch ? I18n::text(6667) /* EN: On */ : I18n::text(6668) /* EN: Off */);

  setValue(m_rowThrottleAngle,
           QString::number(data->throttle_angle),
           QString::number(data->throttle_angle * 0.6, 'f', 1) + " °");

  setValue(m_rowUk6, QString::number(data->uk6), QString::number(data->uk6));

  setValue(m_rowAirFuelRatio,
           QString::number(data->air_fuel_ratio),
           QString::number(data->air_fuel_ratio / 10.0, 'f', 1) + " AFR");

  setValue(m_rowDtc2,
           QString::number(data->dtc2),
           data->dtc2 != 0 ? I18n::text(6669) /* EN: FAULTS */ : I18n::text(6670) /* EN: no fault */);

  setValue(m_rowLambdaVoltage,
           QString::number(data->lambda_voltage),
           QString::number(data->lambda_voltage * 5) + " mV");

  setValue(m_rowLambdaFrequency,
           QString::number(data->lambda_sensor_frequency),
           QString::number(data->lambda_sensor_frequency));

  setValue(m_rowLambdaDutycycle,
           QString::number(data->lambda_sensor_dutycycle),
           QString::number(data->lambda_sensor_dutycycle) + " %");

  setValue(m_rowLambdaStatus,
           QString::number(data->lambda_sensor_status),
           data->lambda_sensor_status == 1 ? I18n::text(6671) /* EN: Sensor OK */ : I18n::text(6672) /* EN: SENSOR FAULT */);

  setValue(m_rowClosedLoop,
           QString::number(data->closed_loop),
           data->closed_loop ? I18n::text(6673) /* EN: Closed loop */ : I18n::text(6674) /* EN: Open loop */);

  setValue(m_rowLongTermFuelTrim,
           QString::number(data->long_term_fuel_trim),
           QString::number(data->long_term_fuel_trim - 128));

  setValue(m_rowShortTermFuelTrim,
           QString::number(data->short_term_fuel_trim),
           QString::number(data->short_term_fuel_trim - 100) + " %");

  setValue(m_rowCarbonCanisterDutycycle,
           QString::number(data->carbon_canister_dutycycle),
           QString::number(data->carbon_canister_dutycycle) + " %");

  setValue(m_rowDtc3,
           QString::number(data->dtc3),
           data->dtc3 != 255 ? I18n::text(6675) /* EN: FAULTS */ : I18n::text(6676) /* EN: no fault */);

  setValue(m_rowIdleBasePos,
           QString::number(data->idle_base_pos),
           QString::number(data->idle_base_pos));

  setValue(m_rowUk7,
           QString::number(data->uk7),
           data->uk7 != 255 ? I18n::text(6677) /* EN: FAULTS */ : I18n::text(6678) /* EN: no fault */);

  setValue(m_rowDtc4,
           QString::number(data->dtc4),
           data->dtc4 != 255 ? I18n::text(6679) /* EN: FAULTS */ : I18n::text(6680) /* EN: no fault */);

  setValue(m_rowIgnitionAdvance2,
           QString::number(data->ignition_advance2),
           QString::number(data->ignition_advance2 - 48) + " °");

  setValue(m_rowIdleSpeedOffset,
           QString::number(data->idle_speed_offset),
           QString::number((data->idle_speed_offset - 128) * 25) + " RPM");

  const int raw7d1415 =
      (static_cast<int>(data->idle_error2) << 8) |
       static_cast<int>(data->uk10);
  const int idleHotCorrection = static_cast<int>(data->idle_hot) - 35;
  const int idleErrorHotCorrected =
      (raw7d1415 - 32768) + idleHotCorrection;

  setValue(m_rowIdleErrorHotCorrected,
           QString::number(raw7d1415),
           QString::number(idleErrorHotCorrected));

  setValue(m_rowUk10,
           QString::number(raw7d1415),
           QString::number(raw7d1415));

  setValue(m_rowDtc5,
           QString::number(data->dtc5),
           data->dtc5 != 255 ? I18n::text(6681) /* EN: FAULTS */ : I18n::text(6682) /* EN: no fault */);

  setValue(m_rowUk11, QString::number(data->uk11), QString::number(data->uk11));
  setValue(m_rowUk12, QString::number(data->uk12), QString::number(data->uk12));
  setValue(m_rowUk13, QString::number(data->uk13), QString::number(data->uk13));
  setValue(m_rowUk14, QString::number(data->uk14), QString::number(data->uk14));
  setValue(m_rowUk15, QString::number(data->uk15), QString::number(data->uk15));
  setValue(m_rowUk16, QString::number(data->uk16), QString::number(data->uk16));
  setValue(m_rowUk1A, QString::number(data->uk1A), QString::number(data->uk1A));
  setValue(m_rowUk1B, QString::number(data->uk1B), QString::number(data->uk1B));
  setValue(m_rowUk1C, QString::number(data->uk1C), QString::number(data->uk1C));

  setValue(m_rowDtc0,
           QString::number(data->dtc0),
           data->dtc0 != 0 ? I18n::text(6683) /* EN: FAULTS */ : I18n::text(6684) /* EN: no fault */);
  setValue(m_rowDtc1,
           QString::number(data->dtc1),
           data->dtc1 != 0 ? I18n::text(6685) /* EN: FAULTS */ : I18n::text(6686) /* EN: no fault */);
}
