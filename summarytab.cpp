#include "summarytab.h"
#include "applicationvisualstyle.h"
#include <QApplication>
#include <QHeaderView>
#include <QFontMetrics>
#include <QIcon>
#include <QResizeEvent>
#include <QTimer>
#include <QSize>
#include "i18n.h"
#include "mems16na_presentation.h"
#include "mems219_units.h"
#include "mems220_presentation.h"
#include "memsextendeddtc_presentation.h"
#include "tooltipbubblemanager.h"
#define tr I18n::text

// The longest visible Parameter label determines the natural first-column
// width.  Spare pixels go to "Interpreted", not back into Parameter; no
// transport values, translations, tooltips or ECU handlers are changed.
static void sizeSummaryTable(QTableWidget *table)
{
  if (!table || table->columnCount() < 4) return;
  QHeaderView *header=table->horizontalHeader();
  const int width=table->viewport()->width()-2;
  if(width<=0) return;
  header->setStretchLastSection(false);
  header->setMinimumSectionSize(0);
  for(int col=0;col<4;++col)
    header->setSectionResizeMode(col,QHeaderView::Fixed);

  const QFontMetrics rowMetrics(table->font());
  int longest=rowMetrics.horizontalAdvance(
    table->horizontalHeaderItem(0)?table->horizontalHeaderItem(0)->text():QString());
  for(int row=0;row<table->rowCount();++row) {
    if(QTableWidgetItem *item=table->item(row,0))
      longest=qMax(longest,rowMetrics.horizontalAdvance(item->text()));
  }

  const int helpWidth=30;
  const int ecuWidth=70;
  // Reserve enough room for the translated Interpreted header and values.
  const int interpretedMinimum=qMax(106,QFontMetrics(header->font()).horizontalAdvance(
    table->horizontalHeaderItem(3)?table->horizontalHeaderItem(3)->text():QString())+18);
  const int parameterNatural=longest+18;
  const int parameterAvailable=qMax(45,width-helpWidth-ecuWidth-interpretedMinimum);
  const int parameterWidth=qMin(parameterNatural,parameterAvailable);
  const int interpretedWidth=qMax(interpretedMinimum,width-parameterWidth-helpWidth-ecuWidth);

  header->resizeSection(0,parameterWidth);
  header->resizeSection(1,helpWidth);
  header->resizeSection(2,ecuWidth);
  header->resizeSection(3,interpretedWidth);
}

SummaryTab::SummaryTab(QWidget *parent) : QWidget(parent)
{
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(5,5,5,5);
  layout->setSpacing(5);
  m_rowCount = 0;
  m_rowsPerTable = 19;

  m_table0 = new QTableWidget(0, 4, this);
  m_table1 = new QTableWidget(0, 4, this);
  m_table2 = new QTableWidget(0, 4, this);

  QTableWidget *tables[3] = { m_table0, m_table1, m_table2 };
  for (int i = 0; i < 3; i++)
  {
    // Typography and colours are owned by applyVisualTheme() below.
    tables[i]->setHorizontalHeaderLabels(QStringList()
        << I18n::text(6600) /* EN: Parameter */ << "" << I18n::text(6601) /* EN: ECU received */ << I18n::text(6602) /* EN: Interpreted */);
    tables[i]->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    tables[i]->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    tables[i]->setColumnWidth(1, 28);
    tables[i]->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    tables[i]->setColumnWidth(2, 54);
    tables[i]->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    tables[i]->setColumnWidth(3, 82);
    tables[i]->verticalHeader()->setVisible(false);
    tables[i]->setEditTriggers(QTableWidget::NoEditTriggers);
    tables[i]->setSelectionMode(QTableWidget::NoSelection);
    tables[i]->setAlternatingRowColors(true);
    tables[i]->setShowGrid(false);
    tables[i]->setIconSize(QSize(TooltipBubbleManager::helpIconSize(),TooltipBubbleManager::helpIconSize()));
    if (QTableWidgetItem *head=tables[i]->horizontalHeaderItem(2)) {
      QString value=head->text();
      if (!value.contains(QLatin1Char('\n'))) {
        const int space=value.indexOf(QLatin1Char(' '));
        if (space>0) value[space]=QLatin1Char('\n');
      }
      head->setText(value);
      head->setTextAlignment(Qt::AlignCenter);
    }
    if (QTableWidgetItem *head=tables[i]->horizontalHeaderItem(3))
      head->setTextAlignment(Qt::AlignCenter);
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
  m_rowLongTermFuelTrim      = addRow(I18n::text(2971701));
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
    I18n::text(6698) /* EN: Decodes field 7D14-15: the raw 16-bit value is centred on 32768, then the hot-idle position correction configured in the Settings tab is applied. Formula: (raw value - 32768) - hot-idle correction. The correction is not constant; it follows the user's current setting. */);
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

  showNaDocumentation();
  applyVisualTheme(qApp && qApp->property("ecuDarkTheme").toBool());
  for (QTableWidget *table : {m_table0,m_table1,m_table2})
    sizeSummaryTable(table);
  QTimer::singleShot(0,this,[this]() {
    for (QTableWidget *table : {m_table0,m_table1,m_table2})
      sizeSummaryTable(table);
  });
}

void SummaryTab::applyVisualTheme(bool dark)
{
  // Each of these three tables owns its approved 10 pt Segoe UI typography.
  // Reapply it after the application palette changes, as Qt may repolish
  // header widgets when the global stylesheet is replaced.
  const QString bodyStyle=dark
    ?QStringLiteral("QTableWidget{color:#ffffff;font-weight:400;}"
                    "QTableWidget:disabled{color:#ffffff;}"
                    "QTableWidget::item{color:#ffffff;}"
                    "QTableWidget::item:disabled{color:#ffffff;}")
    :QStringLiteral("QTableWidget{color:#202936;font-weight:400;}"
                    "QTableWidget:disabled{color:#536172;}"
                    "QTableWidget::item{color:#202936;}"
                    "QTableWidget::item:disabled{color:#536172;}");
  const QString headerStyle=dark
    ?QStringLiteral(
        "QHeaderView{font-family:\"Segoe UI\";font-size:10pt;font-weight:700;}"
        "QHeaderView::section{background:#141c23;color:#ff9828;border:0;"
        "border-right:1px solid #29343e;border-bottom:2px solid #ff7a00;"
        "padding:4px 5px;font-weight:700;}")
    :QStringLiteral(
        "QHeaderView{font-family:\"Segoe UI\";font-size:10pt;font-weight:700;}"
        "QHeaderView::section{background:#edf2f6;color:#283748;border:0;"
        "border-bottom:1px solid #cbd5df;padding:5px 7px;font-weight:700;}");
  for(QTableWidget *table:{m_table0,m_table1,m_table2}){
    table->setStyleSheet(bodyStyle);
    table->setFont(DataPageTypography::body(table->font()));
    QHeaderView *header=table->horizontalHeader();
    header->setStyleSheet(headerStyle);
    header->setFont(DataPageTypography::header(table->font()));
  }
}

void SummaryTab::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  QTimer::singleShot(0,this,[this]() {
    for (QTableWidget *table : {m_table0,m_table1,m_table2})
      sizeSummaryTable(table);
  });
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
  // Item text follows the active dark/light palette, never fixed white.
  for (int col=1; col<4; ++col)
    if (QTableWidgetItem *item=table->item(localRow,col))
      item->setTextAlignment(Qt::AlignCenter);
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

void SummaryTab::updateData(mems_data *data,const Mems16Na::SessionEvidence *na)
{
  if (!data) return;
  if(!na||!MemsExtendedDtc::short13(na->firmwareName().c_str()))clear13Analogue();
  if(!na||!Mems220::profile19(*na))clear220Presentation();
  if (!na || !na->identifiedProfile())clearNaPresentation();
  if (m_showing2J)
  {
    for (int globalRow=0; globalRow<m_rowCount; ++globalRow)
    {
      QTableWidget *table=(globalRow/m_rowsPerTable==0)?m_table0:
          (globalRow/m_rowsPerTable==1)?m_table1:m_table2;
      const int row=globalRow%m_rowsPerTable;
      table->setRowHidden(row,false);
      if (QTableWidgetItem *item=table->item(row,0))
      {
        item->setText(m_savedRowLabels.value(globalRow));
        const QString tip=m_savedRowTooltips.value(globalRow);
        for(int col=0;col<4;++col)table->item(row,col)->setToolTip(tip);
        table->item(row,1)->setIcon(tip.isEmpty()?QIcon():QIcon(":/icons/helpbubble.png"));
      }
    }
    m_showing2J=false;
  }
  if(na&&na->identifiedProfile()&&!na->ready()) {
    applyNaPresentation(*na);
    return;
  }

  // Colonne 1 : valeur reçue directement de l'ECU.
  // Colonne 2 : valeur interprétée selon les mêmes conversions que l'onglet
  // « Toutes les données ».

  setValue(m_rowEngineRpm,
           QString::number(data->engine_rpm),
           QString::number(data->engine_rpm) + " RPM");

  setValue(m_rowCoolantTemp,
           QString::number(data->coolant_temp),
           Mems219::temperatureAvailable(data->coolant_temp)?QString::number(Mems219::temperatureC(data->coolant_temp))+QStringLiteral(" °C"):I18n::text(29002));
  setValue(m_rowAmbientTemp,
           QString::number(data->ambient_temp),
           Mems219::temperatureAvailable(data->ambient_temp)?QString::number(Mems219::temperatureC(data->ambient_temp))+QStringLiteral(" °C"):I18n::text(29002));
  setValue(m_rowIntakeAirTemp,
           QString::number(data->intake_air_temp),
           Mems219::temperatureAvailable(data->intake_air_temp)?QString::number(Mems219::temperatureC(data->intake_air_temp))+QStringLiteral(" °C"):I18n::text(29002));
  setValue(m_rowFuelTemp,
           QString::number(data->fuel_temp),
           Mems219::temperatureAvailable(data->fuel_temp)?QString::number(Mems219::temperatureC(data->fuel_temp))+QStringLiteral(" °C"):I18n::text(29002));

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
           na&&Mems217::injectionProfile(*na)&&Mems217::present(Mems217::frame80(*na),16)?QString::number(Mems217::frame80(*na).bytes[16])+QStringLiteral(" ")+I18n::text(29065):QString::number(data->idle_hot - 35)+QStringLiteral(" ")+I18n::text(29065));

  setValue(m_rowUk2, QString::number(data->uk2), QString::number(data->uk2));

  setValue(m_rowIacPosition,
           QString::number(data->iac_position),
           QString::number(data->iac_position)+QStringLiteral(" ")+I18n::text(29065));

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
  if(na&&MemsExtendedDtc::short13(na->firmwareName().c_str())){
    const int i=m_rowUk6;auto *table=i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;
    auto *label=table->item(i%m_rowsPerTable,0);
    const bool first=!m_showing13Analogue;
    if(first){m_13AnalogueLabel=label->text();m_13AnalogueTip=label->toolTip();m_showing13Analogue=true;}
    label->setText(I18n::text(2972312));setTooltip(i,I18n::text(2972313));
    if(first)sizeSummaryTable(table);
    const auto &b=na->extended7d();
    const bool valid=MemsExtendedDtc::analogue13Available(na->firmwareName().c_str(),b.data(),b.size())&&b[3]==data->uk6;
    setValue(i,valid?QString::number(b[3]):QStringLiteral("--"),QStringLiteral("--"));
  }

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

  const bool knownInjection=na&&Mems219::injectionAvailable(*na);
  const auto correctionFrame=na?Mems217::frame7d(*na):Mems16Na::Frame{nullptr,0};
  setValue(m_rowLongTermFuelTrim,QString::number(data->long_term_fuel_trim),knownInjection?QString::number(Mems217::injectionUs(correctionFrame.bytes[11]))+QStringLiteral(" µs"):I18n::text(29002));

  setValue(m_rowShortTermFuelTrim,
           QString::number(data->short_term_fuel_trim),
           na&&Mems219::lambdaQualified(*na)?QString::number(int(correctionFrame.bytes[12])-100)+QStringLiteral(" %"):I18n::text(29002));

  setValue(m_rowCarbonCanisterDutycycle,
           QString::number(data->carbon_canister_dutycycle),
           QString::number(data->carbon_canister_dutycycle) + " %");

  setValue(m_rowDtc3,MemsExtendedDtc::raw(na,3,data->dtc3),MemsExtendedDtc::text(na,3));

  setValue(m_rowIdleBasePos,
           QString::number(data->idle_base_pos),
           QString::number(data->idle_base_pos));

  setValue(m_rowUk7,MemsExtendedDtc::raw(na,6,data->uk7),MemsExtendedDtc::text(na,6));

  setValue(m_rowDtc4,MemsExtendedDtc::raw(na,4,data->dtc4),MemsExtendedDtc::text(na,4));

  setValue(m_rowIgnitionAdvance2,
           QString::number(data->ignition_advance2),
           QString::number(Mems219::ignitionAdjustmentDeg(data->ignition_advance2),'f',1)+QStringLiteral(" °"));

  setValue(m_rowIdleSpeedOffset,
           QString::number(data->idle_speed_offset),
           QString::number((data->idle_speed_offset - 128) * 25) + " RPM");

  const int raw7d1415 =
      (static_cast<int>(data->idle_error2) << 8) |
       static_cast<int>(data->uk10);
  const int idleHotCorrection = static_cast<int>(data->idle_hot) - 35;
  const int idleErrorHotCorrected =
      (raw7d1415 - 32768) - idleHotCorrection;

  setValue(m_rowIdleErrorHotCorrected,
           QString::number(raw7d1415),
           QString::number(idleErrorHotCorrected));

  setValue(m_rowUk10,
           QString::number(raw7d1415),
           QString::number(raw7d1415));

  setValue(m_rowDtc5,MemsExtendedDtc::raw(na,5,data->dtc5),MemsExtendedDtc::text(na,5));

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
  if(na && na->identifiedProfile())applyNaPresentation(*na);
  else setValue(m_rowDtc2,QString::number(MemsExtendedDtc::diagnosticDtc2(na,data->dtc2).raw),MemsExtendedDtc::diagnosticDtc2Text(na,data->dtc2));
  if(na&&Mems220::profile19(*na))apply220Presentation(*na);
}


void SummaryTab::update2JData(const QVariantMap &values, const QString &family)
{
  clear13Analogue();
  clear220Presentation();
  clearNaPresentation();
  // MEMS 2J sends 0x21/0x61 measurements, not ROSCO 0x80/0x7D.
  // Reuse the existing three measurement tables without mixing the units.
  struct Field { const char *key; int label; const char *unit; };
  static const Field fields2J[] = {
    {"rpm",29120,"tr/min"},
    {"coolant_c",29121,"°C"},
    {"intake_c",29122,"°C"},
    {"oil_c",29123,"°C"},
    {"map_kpa",29124,"kPa"},
    {"throttle_deg",29125,"°"},
    {"battery_v",29126,"V"},
    {"lambda_feedback_percent",29127,"%"},
    {"lambda_o2_raw",29128,""},
    {"fuel_raw",29129,""},
    {"idle_steps",29130,""},
    {"idle_target_rpm",29131,"tr/min"},
    {"idle_offset_raw10",29132,""},
    {"rpm_error",29133,"tr/min"},
    {"speed_raw",29134,""},
    {"coil1_raw",29135,""},
    {"coil2_raw",29136,""},
    {"injector1_raw",29137,""},
    {"injector2_raw",29138,""},
    {"injector3_raw",29139,""},
    {"injector4_raw",29140,""},
    {"ignition_on",29141,"0/1"},
    {"throttle_closed",29142,"0/1"},
    {"ac_requested",29143,"0/1"},
    {"primary_sync",29144,"0/1"},
    {"secondary_sync",29145,"0/1"},
    {"closed_loop",29146,"0/1"},
    {"cam_raw",29147,""},
    {"faults_raw",29148,""},
    {"DATA_00_RAW",29149,""},
    {"DATA_06_RAW",29150,""}
  };
  static const Field fields3[] = {
    {"rpm",29120,"tr/min"},
    {"coolant_c",29121,"°C"},
    {"oil_c",29123,"°C"},
    {"intake_c",29122,"°C"},
    {"map_kpa",29124,"kPa"},
    {"throttle_mv",29151,"mV"},
    {"lambda_feedback_percent",29127,"%"},
    {"lambda_mv",29152,"mV"},
    {"coil1_us",29153,"µs"},
    {"coil2_us",29154,"µs"},
    {"rpm_deviation_raw",29155,""},
    {"faults_raw",29148,""}
  };
  const bool is3=family==QStringLiteral("MEMS 3");
  const Field *fields=is3?fields3:fields2J;
  const int count=is3
      ? int(sizeof(fields3)/sizeof(fields3[0]))
      : int(sizeof(fields2J)/sizeof(fields2J[0]));
  const bool refreshColumns=!m_showing2J || m_table0->isRowHidden(12)!=is3;
  if (!m_showing2J)
  {
    m_savedRowLabels.clear(); m_savedRowTooltips.clear();
    for (int globalRow=0; globalRow<m_rowCount; ++globalRow)
    {
      QTableWidget *table=(globalRow/m_rowsPerTable==0)?m_table0:
          (globalRow/m_rowsPerTable==1)?m_table1:m_table2;
      QTableWidgetItem *item=table->item(globalRow%m_rowsPerTable,0);
      m_savedRowLabels.append(item?item->text():QString());
      m_savedRowTooltips.append(item?item->toolTip():QString());
    }
    m_showing2J=true;
  }
  for (int globalRow=0;globalRow<m_rowCount;++globalRow)
  {
    QTableWidget *table=(globalRow/m_rowsPerTable==0)?m_table0:
        (globalRow/m_rowsPerTable==1)?m_table1:m_table2;
    const int row=globalRow%m_rowsPerTable;
    table->setRowHidden(row,globalRow>=count);
    if (globalRow>=count) continue;
    const Field &f=fields[globalRow];
    if (QTableWidgetItem *item=table->item(row,0))
    {
      item->setText(I18n::text(f.label));
    }
    const QString key=QString::fromLatin1(f.key);
    const bool rawOnly=key.endsWith(QStringLiteral("_raw"),Qt::CaseInsensitive);
    const QString unit=QString::fromUtf8(f.unit);
    const QString tip=I18n::text(f.label)+QStringLiteral("\n")+(rawOnly?I18n::text(29112):(unit.isEmpty()?I18n::text(29114):unit));
    setTooltip(globalRow,tip);
    if (!values.contains(key))
    {
      setValue(globalRow,QStringLiteral("--"),QStringLiteral("--"));
      continue;
    }
    if (key==QStringLiteral("faults_raw"))
    {
      const QString raw=values.value(key).toString();
      setValue(globalRow,raw,I18n::text(29113));
      table->item(row,2)->setToolTip(raw);
    }
    else if(rawOnly)
    {
      const QString raw=values.value(key).toString();
      setValue(globalRow,raw,QStringLiteral("--"));
      table->item(row,2)->setToolTip(raw);
    }
    else
    {
      // The decoder preserves the exact original integer independently of scaling.
      const QVariantMap received=values.value(QStringLiteral("_received")).toMap();
      const QString raw=received.contains(key)?received.value(key).toString():QStringLiteral("--");
      const QString value=QString::number(values.value(key).toDouble(),'f',
                          key==QStringLiteral("battery_v")?3:2);
      setValue(globalRow,raw,unit.isEmpty()?value:QStringLiteral("%1 %2").arg(value,unit));
    }
  }
  if(refreshColumns)for(QTableWidget *table:{m_table0,m_table1,m_table2})sizeSummaryTable(table);
}

void SummaryTab::clear13Analogue()
{
  if(!m_showing13Analogue)return;
  const int i=m_rowUk6;auto *table=i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;
  table->item(i%m_rowsPerTable,0)->setText(m_13AnalogueLabel);setTooltip(i,m_13AnalogueTip);
  setValue(i,QStringLiteral("--"),QStringLiteral("--"));m_showing13Analogue=false;
  sizeSummaryTable(table);
}
void SummaryTab::clear220Presentation()
{
  if(!m_showing220)return;
  const int rows[]={m_rowIdleHot,m_rowUk2,m_rowIdleSpeedOffset,m_rowIdleErrorHotCorrected,
      m_rowUk11,m_rowUk12,m_rowUk13,m_rowUk14,m_rowUk15,m_rowUk16,m_rowUk1A,m_rowUk1B,m_rowUk1C,
      m_rowUk1,m_rowParkNeutralSwitch,m_rowUk5,m_rowIgnitionSwitch};
  for(int j=0;j<int(sizeof(rows)/sizeof(rows[0]));++j){
    const int i=rows[j];auto *t=i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;
    t->item(i%m_rowsPerTable,0)->setText(m_220Labels.value(j));
    setTooltip(i,m_220Tips.value(j));setValue(i,QStringLiteral("--"),QStringLiteral("--"));
  }
  m_showing220=false;
  for(QTableWidget *t:{m_table0,m_table1,m_table2})sizeSummaryTable(t);
}
void SummaryTab::apply220Presentation(const Mems16Na::SessionEvidence &s)
{
  using namespace Mems16Na;
  const auto m=Mems220::decode(s);
  auto table=[&](int i){return i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;};
  auto item=[&](int i,int col){return table(i)->item(i%m_rowsPerTable,col);};
  const int rows[]={m_rowIdleHot,m_rowUk2,m_rowIdleSpeedOffset,m_rowIdleErrorHotCorrected,
      m_rowUk11,m_rowUk12,m_rowUk13,m_rowUk14,m_rowUk15,m_rowUk16,m_rowUk1A,m_rowUk1B,m_rowUk1C,
      m_rowUk1,m_rowParkNeutralSwitch,m_rowUk5,m_rowIgnitionSwitch};
  const bool first=!m_showing220;
  if(first){
    m_220Labels.clear();m_220Tips.clear();
    for(int i:rows){m_220Labels<<item(i,0)->text();m_220Tips<<item(i,0)->toolTip();}
    m_showing220=true;
  }
  auto field=[&](int i,int title,const Value &v,const QString &value,int help){
    item(i,0)->setText(I18n::text(title));setValue(i,rawText(v),value);setTooltip(i,I18n::text(help));
  };
  field(m_rowUk1,2972301,m.dn,Mems220::inputState(m.dn),2972307);
  field(m_rowParkNeutralSwitch,2972302,m.ac,Mems220::inputState(m.ac),2972307);
  field(m_rowUk5,2972303,m.ignition80,Mems220::inputState(m.ignition80),2972307);
  field(m_rowIgnitionSwitch,2972303,m.ignition7d,Mems220::inputState(m.ignition7d),2972307);
  field(m_rowIdleHot,29025,m.hotPosition,steps(m.hotPosition),29054);
  field(m_rowUk2,29026,m.learnVoltage,Mems220::voltage(m),29054);
  field(m_rowIdleSpeedOffset,29024,m.idleCorrection,numeric(m.idleCorrection,0,QStringLiteral(" RPM")),29054);
  field(m_rowIdleErrorHotCorrected,29031,m.idleError,Mems220::idleError(m),29043);
  const int hi[]={m_rowUk11,m_rowUk13,m_rowUk15,m_rowUk1A};
  const int lo[]={m_rowUk12,m_rowUk14,m_rowUk16,m_rowUk1B};
  const auto b=Mems217::frame7d(s);
  for(int j=0;j<4;++j){
    const QString label=j==0?I18n::text(2972001):I18n::text(2972002).arg(j+1);
    item(hi[j],0)->setText(label);item(lo[j],0)->setText(label+QStringLiteral(" (LSB)"));
    setValue(hi[j],m.pair?QString::number(b.bytes[23+2*j]):QStringLiteral("--"),j==0?Mems220::reference(m):QStringLiteral("--"));
    setValue(lo[j],m.pair?QString::number(b.bytes[24+2*j]):QStringLiteral("--"),QStringLiteral("--"));
    setTooltip(hi[j],I18n::text(2972004));setTooltip(lo[j],I18n::text(2972004));
  }
  field(m_rowUk1C,29033,m.learnCount,Mems220::count(m),29054);
  if(first)for(QTableWidget *t:{m_table0,m_table1,m_table2})sizeSummaryTable(t);
}

void SummaryTab::clearNaPresentation()
{
  if(!m_showingNa)return;
  for(int i=0;i<m_rowCount;++i) {
    QTableWidget *t=i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;
    const int row=i%m_rowsPerTable;
    t->setRowHidden(row,false);
    t->item(row,0)->setText(m_naSavedLabels.value(i));
    const QString tip=m_naSavedTooltips.value(i);
    for(int col=0;col<4;++col)t->item(row,col)->setToolTip(tip);
    t->item(row,1)->setIcon(tip.isEmpty()?QIcon():QIcon(":/icons/helpbubble.png"));
    setValue(i,QStringLiteral("--"),QStringLiteral("--"));
  }
  m_showingNa=false;
  for(QTableWidget *t:{m_table0,m_table1,m_table2})sizeSummaryTable(t);
}

void SummaryTab::showNaDocumentation()
{
  clear13Analogue();
  clear220Presentation();
  if(m_showing2J) {
    for(int i=0;i<m_rowCount;++i) {
      QTableWidget *table=i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;
      const int row=i%m_rowsPerTable;
      table->setRowHidden(row,false);
      table->item(row,0)->setText(m_savedRowLabels.value(i));
      const QString tip=m_savedRowTooltips.value(i);
      for(int col=0;col<4;++col)table->item(row,col)->setToolTip(tip);
      table->item(row,1)->setIcon(tip.isEmpty()?QIcon():QIcon(":/icons/helpbubble.png"));
    }
    m_showing2J=false;
  }
  clearNaPresentation();
  applyNaPresentation(Mems16Na::SessionEvidence(),true);
}

void SummaryTab::applyNaPresentation(const Mems16Na::SessionEvidence &na,bool documentationOnly)
{
  using namespace Mems16Na;
  const auto *p=na.identifiedProfile();
  if(!p&&!documentationOnly)return;
  auto table=[this](int i){return i/m_rowsPerTable==0?m_table0:i/m_rowsPerTable==1?m_table1:m_table2;};
  auto item=[&](int i,int col){return table(i)->item(i%m_rowsPerTable,col);};
  const bool first=!m_showingNa;
  if(first) {
    m_naSavedLabels.clear();m_naSavedTooltips.clear();
    for(int i=0;i<m_rowCount;++i) {
      m_naSavedLabels<<item(i,0)->text();
      m_naSavedTooltips<<item(i,0)->toolTip();
    }
    m_showingNa=true;
  }
  for(int i=0;i<m_rowCount;++i) {
    if(!na.ready()) {
      setValue(i,QStringLiteral("--"),I18n::text(29048));
      item(i,3)->setToolTip(I18n::text(29048));
    } else item(i,3)->setToolTip(m_naSavedTooltips.value(i));
  }
  auto row=[&](int i,int key,const Value &v,const QString &out,const QString &tip=QString()){
    item(i,0)->setText(I18n::text(key));
    setValue(i,rawText(v),out);
    const QString detail=tip.isEmpty()?I18n::text(key):tip;
    setTooltip(i,documentationOnly?documentation(detail):detail);
    item(i,3)->setToolTip(documentationOnly?documentation(detail):out+QStringLiteral("\n")+detail);
  };
  const auto m=na.measurements();
  row(m_rowAmbientTemp,29046,m.ambient,numeric(m.ambient,0,QStringLiteral(" °C")));
  row(m_rowFuelTemp,29047,m.fuel,numeric(m.fuel,0,QStringLiteral(" °C")));
  row(m_rowThrottlePot,29027,m.throttleVoltage,numeric(m.throttleVoltage,2,QStringLiteral(" V")));
  row(m_rowUk1,29022,m.pn,m.pn.availability==Availability::Available?
      (m.pn.calculated?QStringLiteral("P/N"):I18n::text(29035)):statusText(m.pn),I18n::text(29045));
  row(m_rowParkNeutralSwitch,29023,m.ac,state(m.ac,29036,29037),I18n::text(29045));
  row(m_rowIdleSetPoint,29024,m.idleAmplitude,idleCorrection(m));
  row(m_rowIdleHot,29025,m.idleHot,steps(m.idleHot));
  row(m_rowUk2,29026,m.learnVoltage,numeric(m.learnVoltage,1,QStringLiteral(" V")));
  row(m_rowUk5,29028,m.ignitionOff,state(m.ignitionOff,29038,29039),I18n::text(29045));
  row(m_rowThrottleAngle,6627,m.throttleAngle,numeric(m.throttleAngle,1,QStringLiteral(" °")));
  const auto signedIdle=qualifiedIdleSigned(m);
  row(m_rowIdleSpeedOffset,6644,signedIdle,numeric(signedIdle,0,QStringLiteral(" RPM")),I18n::text(29054));
  row(m_rowLambdaVoltage,6631,m.lambdaMv,numeric(m.lambdaMv,0,QStringLiteral(" mV")),I18n::text(6702));
  row(m_rowLambdaStatus,6634,m.lambdaFault,state(m.lambdaFault,6672,6671),
      I18n::text(29053).arg(state(m.lambdaFault,29038,29039)));
  row(m_rowClosedLoop,6635,m.closedLoop,state(m.closedLoop,6673,6674),I18n::text(6704));
  row(m_rowLongTermFuelTrim,29029,m.injectionTime,numeric(m.injectionTime,0,QStringLiteral(" µs")),I18n::text(29044));
  row(m_rowIdleBasePos,29030,m.idleBase,steps(m.idleBase));
  row(m_rowIdleErrorHotCorrected,29031,m.idleError,numeric(m.idleError,0,QStringLiteral(" RPM")),I18n::text(29043));
  const int hi[]={m_rowUk11,m_rowUk13,m_rowUk15,m_rowUk1A};
  const int lo[]={m_rowUk12,m_rowUk14,m_rowUk16,m_rowUk1B};
  QString refHelp=I18n::text(29041);
  if(p&&p->repeatedThrottleReference)refHelp+=QStringLiteral("\n")+I18n::text(29042);
  if(documentationOnly)refHelp=documentation(refHelp);
  for(int i=0;i<4;++i) {
    item(hi[i],0)->setText(I18n::text(29032).arg(i+1));
    setValue(hi[i],rawText(m.throttleReference[i]),numeric(m.throttleReference[i],2,QStringLiteral(" TPS")));
    setTooltip(hi[i],refHelp);
    // Retain the original low byte as a separate received value.
    const Value &v=m.throttleReference[i];
    item(lo[i],0)->setText(I18n::text(29032).arg(i+1)+QStringLiteral(" (LSB)"));
    setValue(lo[i],v.availability==Availability::Missing?QStringLiteral("--"):QString::number(v.raw&255),QStringLiteral("--"));
    setTooltip(lo[i],refHelp);
    table(lo[i])->setRowHidden(lo[i]%m_rowsPerTable,false);
  }
  item(m_rowUk10,0)->setText(I18n::text(29111));
  setValue(m_rowUk10,rawText(m.idleError),QStringLiteral("--"));
  setTooltip(m_rowUk10,I18n::text(29112));
  table(m_rowUk10)->setRowHidden(m_rowUk10%m_rowsPerTable,false);
  row(m_rowUk1C,29033,m.learnCount,numeric(m.learnCount,0));
  const int rows[]={m_rowDtc0,m_rowDtc1,m_rowDtc2,m_rowDtc3,m_rowDtc4,m_rowDtc5,m_rowUk7};
  const Group groups[]={Group::Dtc0,Group::Dtc1,Group::Dtc2,Group::Dtc3,Group::Dtc4,Group::Dtc5,Group::Indirect7D10};
  for(int i=0;i<7;++i) {
    const auto d=na.dtc(groups[i]);
    const QString help=documentationOnly?dtcDocumentation(groups[i]):dtcHelp(p,groups[i]);
    const QString result=documentationOnly?QStringLiteral("--"):dtcText(p,groups[i],d);
    setValue(rows[i],rawDtc(d),result);
    setTooltip(rows[i],documentationOnly?help:result+QStringLiteral("\n\n")+help);
  }
  // Keep the established readable descriptions, replacing only generic
  // placeholders with the matching raw-view name or exact byte position.
  item(m_rowUk3,0)->setText(I18n::text(5198));
  item(m_rowUk4,0)->setText(I18n::text(29079));
  item(m_rowUk6,0)->setText(I18n::text(29080));
  item(m_rowUk7,0)->setText(I18n::text(29081));
  setTooltip(m_rowUk3,documentation(I18n::text(29076)));
  setTooltip(m_rowUk4,documentation(I18n::text(29077)));
  for(int i:{m_rowUk6,m_rowLambdaFrequency,m_rowLambdaDutycycle})
    setTooltip(i,documentation(I18n::text(29078)));
  if(documentationOnly) {
    for(int i=0;i<m_rowCount;++i)setValue(i,QStringLiteral("--"),QStringLiteral("--"));
    for(int i=0;i<7;++i)
      if(documentedUnusedDtc(groups[i]))setValue(rows[i],QStringLiteral("--"),I18n::text(29082));
  }
  if(first)for(QTableWidget *t:{m_table0,m_table1,m_table2})sizeSummaryTable(t);
}
