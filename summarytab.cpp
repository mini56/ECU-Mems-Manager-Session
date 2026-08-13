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
        << tr("Paramètre") << "" << tr("Reçu ECU") << tr("Interprété"));
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

  m_rowEngineRpm             = addRow(tr("Régime moteur (tr/min)"));
  m_rowCoolantTemp           = addRow(tr("Température liquide refroid. (°C)"));
  m_rowAmbientTemp           = addRow(tr("Température ambiante (°C)"));
  m_rowIntakeAirTemp         = addRow(tr("Température air admission (°C)"));
  m_rowFuelTemp              = addRow(tr("Température carburant (°C)"));
  m_rowMapKpa                = addRow(tr("Pression collecteur (kPa)"));
  m_rowBatteryVoltage        = addRow(tr("Tension batterie (V)"));
  m_rowThrottlePot           = addRow(tr("Position papillon (%)"));
  m_rowIdleSwitch            = addRow(tr("Contact ralenti"));
  m_rowUk1                   = addRow(tr("Non documenté 1"));
  m_rowParkNeutralSwitch     = addRow(tr("Contact point mort"));
  m_rowFaultCodes            = addRow(tr("Codes défaut (brut)"));
  m_rowIdleSetPoint          = addRow(tr("Consigne de ralenti"));
  m_rowIdleHot               = addRow(tr("Ralenti chaud"));
  m_rowUk2                   = addRow(tr("Non documenté 2"));
  m_rowIacPosition           = addRow(tr("Position moteur pas-à-pas (%)"));
  m_rowIdleError             = addRow(tr("Erreur de ralenti"));
  m_rowIgnitionAdvanceOffset = addRow(tr("Décalage d'avance"));
  m_rowIgnitionAdvance       = addRow(tr("Avance à l'allumage (°)"));
  m_rowCoilTime              = addRow(tr("Temps bobine (ms)"));
  m_rowUk3                   = addRow(tr("Non documenté 3"));
  m_rowUk4                   = addRow(tr("Non documenté 4"));
  m_rowUk5                   = addRow(tr("Non documenté 5"));
  m_rowIgnitionSwitch        = addRow(tr("Contact allumage"));
  m_rowThrottleAngle         = addRow(tr("Angle papillon (°)"));
  m_rowUk6                   = addRow(tr("Non documenté 6"));
  m_rowAirFuelRatio          = addRow(tr("Ratio air/carburant"));
  m_rowDtc2                  = addRow(tr("DTC 2"));
  m_rowLambdaVoltage         = addRow(tr("Tension sonde lambda (mV)"));
  m_rowLambdaFrequency       = addRow(tr("Fréquence lambda"));
  m_rowLambdaDutycycle       = addRow(tr("Cycle lambda (%)"));
  m_rowLambdaStatus          = addRow(tr("État lambda"));
  m_rowClosedLoop            = addRow(tr("Boucle fermée"));
  m_rowLongTermFuelTrim      = addRow(tr("Correction carburant long terme (%)"));
  m_rowShortTermFuelTrim     = addRow(tr("Correction carburant court terme (%)"));
  m_rowCarbonCanisterDutycycle = addRow(tr("Cycle purge canister (%)"));
  m_rowDtc3                  = addRow(tr("DTC 3"));
  m_rowIdleBasePos           = addRow(tr("Position de base ralenti"));
  m_rowUk7                   = addRow(tr("Non documenté 7"));
  m_rowDtc4                  = addRow(tr("DTC 4"));
  m_rowIgnitionAdvance2      = addRow(tr("Avance à l'allumage 2"));
  m_rowIdleSpeedOffset       = addRow(tr("Décalage régime de ralenti"));
  m_rowIdleErrorHotCorrected = addRow(tr("Erreur de ralenti à chaud (corrigée)"));
  m_rowUk10                  = addRow(tr("Trame 7D14-15 brute"));
  m_rowDtc5                  = addRow(tr("DTC 5"));
  m_rowUk11                  = addRow(tr("Non documenté 11"));
  m_rowUk12                  = addRow(tr("Non documenté 12"));
  m_rowUk13                  = addRow(tr("Non documenté 13"));
  m_rowUk14                  = addRow(tr("Non documenté 14"));
  m_rowUk15                  = addRow(tr("Non documenté 15"));
  m_rowUk16                  = addRow(tr("Non documenté 16"));
  m_rowUk1A                  = addRow(tr("Non documenté 1A"));
  m_rowUk1B                  = addRow(tr("Non documenté 1B"));
  m_rowUk1C                  = addRow(tr("Non documenté 1C"));
  m_rowDtc0                  = addRow(tr("DTC 0"));
  m_rowDtc1                  = addRow(tr("DTC 1"));

  // Info-bulles explicatives, reprises des textes déjà présents ailleurs
  // dans le logiciel (onglets Aperçu et Réglages). Les lignes concernées
  // sont repérables par l'icône en forme de bulle devant leur nom.
  setTooltip(m_rowCoolantTemp,
    tr("Affiche la température du liquide de refroidissement mesurée par l'ECU. Si le capteur est en circuit ouvert, "
    "une valeur par défaut d'environ 60°C sera affichée. Pendant la montée en température, la valeur doit augmenter "
    "progressivement de la température ambiante à environ 90°C. Un capteur défectueux peut causer un démarrage "
    "difficile, un ralenti trop élevé, une surconsommation et des ventilateurs tournant en continu."));
  setTooltip(m_rowAmbientTemp,
    tr("Affiche la température mesurée par l'ECU via la sonde de température d'air ambiant (si équipée). Si le capteur "
    "est en circuit ouvert, une valeur par défaut fixe sera affichée. Ce capteur sert à l'ECU pour mesurer la "
    "température dans le compartiment moteur, généralement pour piloter un ventilateur supplémentaire. Sur les ECU "
    "MEMS qui ne supportent pas ce capteur, la valeur affichera N/S."));
  setTooltip(m_rowIntakeAirTemp,
    tr("Affiche la température mesurée par l'ECU via la sonde de température d'air admission (si équipée). Si le "
    "capteur est en circuit ouvert, une valeur par défaut fixe sera affichée. Cette température sert à l'ECU pour "
    "retarder l'allumage afin d'éviter le cliquetis et ajuster la richesse à chaud."));
  setTooltip(m_rowMapKpa,
    tr("Affiche la pression mesurée par le capteur de pression d'air interne du MEMS. Cette valeur doit indiquer la "
    "pression atmosphérique de 100 kPa moteur à l'arrêt, et une valeur plus basse entre 25 et 40 kPa au ralenti. Des "
    "valeurs très élevées peuvent indiquer un problème du capteur interne au MEMS, ou plus probablement une durite "
    "de dépression bouchée ou débranchée."));
  setTooltip(m_rowBatteryVoltage,
    tr("Affiche la tension d'alimentation du véhicule mesurée en interne par l'ECU. De grands écarts sur cette mesure "
    "peuvent entraîner des difficultés de démarrage et des erreurs de CO au ralenti."));
  setTooltip(m_rowThrottlePot,
    tr("Affiche la position du papillon obtenue par l'ECU MEMS via le potentiomètre de papillon. Cette valeur doit "
    "passer d'une valeur basse à une valeur haute lorsque la pédale d'accélérateur est enfoncée."));
  setTooltip(m_rowIdleSwitch,
    tr("Affiche l'état du contacteur papillon (si équipé). Si le contact indique 'ON' alors que le papillon est fermé, "
    "le véhicule ne tournera pas correctement au ralenti et la position papillon fermé devra peut-être être "
    "réinitialisée (enfoncer/relâcher complètement l'accélérateur 5 fois en 10 secondes après la mise du contact, "
    "puis attendre 20 secondes)."));
  setTooltip(m_rowParkNeutralSwitch,
    tr("Affiche l'état du contacteur point mort/parking mesuré par l'ECU MEMS. Ce contacteur améliore la régulation du "
    "ralenti sur les véhicules à boîte automatique ou CVT. Ne fonctionne pas sur les véhicules à boîte manuelle."));
  setTooltip(m_rowIdleSpeedOffset,
    tr("Si un décalage de service du ralenti a été configuré dans cet ECU, l'écart par rapport au ralenti normal est "
    "affiché ici. Normalement, seules deux valeurs sont possibles : 0 tr/min ou 49 tr/min."));
  setTooltip(m_rowIacPosition,
    tr("C'est le nombre de pas du moteur pas-à-pas depuis la fermeture complète (0) que l'ECU a appris comme position "
    "correcte pour maintenir le régime de ralenti visé, moteur bien chaud. Si cette valeur sort de la plage 10-50 "
    "pas, c'est le signe possible d'un défaut ou d'un mauvais réglage."));
  setTooltip(m_rowIdleError,
    tr("C'est l'écart actuel entre le régime de ralenti visé par l'ECU MEMS et le régime moteur réel. Une valeur "
    "supérieure à 100 tr/min indique que l'ECU ne maîtrise pas le ralenti, signe possible d'un défaut."));
  setTooltip(m_rowIdleErrorHotCorrected,
    tr("Décodage du champ 7D14-15 : la valeur brute 16 bits est centrée sur 32768, puis la correction de position "
    "du ralenti chaud configurée dans l'onglet Réglages est appliquée. La formule est : "
    "(valeur brute - 32768) + correction ralenti chaud. La correction n'est pas une constante : elle suit le réglage "
    "actuel de l'utilisateur."));
  setTooltip(m_rowIgnitionAdvanceOffset,
    tr("Affiche le décalage d'avance de service actuellement utilisé par l'ECU MEMS. C'est un réglage spécial pour les "
    "pays utilisant un carburant à faible indice d'octane, configurable dans l'onglet Réglages."));
  setTooltip(m_rowCoilTime,
    tr("C'est le temps de charge de la bobine d'allumage jusqu'à son courant nominal, mesuré par l'ECU MEMS. Avec une "
    "tension batterie d'environ 14V, cette valeur doit être d'environ 2-3ms. Une valeur élevée peut indiquer un "
    "problème du circuit primaire de la bobine."));
  setTooltip(m_rowThrottleAngle,
    tr("Affiche la position du papillon obtenue par l'ECU MEMS via le potentiomètre de papillon. Cette valeur doit "
    "passer d'une valeur basse à une valeur haute lorsque la pédale d'accélérateur est enfoncée."));
  setTooltip(m_rowLambdaVoltage,
    tr("Affiche la tension de la sonde à oxygène lue par l'ECU MEMS. Moteur bien chaud, en conditions normales de "
    "ralenti ou de conduite, cette tension oscille rapidement entre 0,0-0,2V et 0,7-1,0V."));
  setTooltip(m_rowLambdaStatus,
    tr("Affiche l'état du diagnostic interne MEMS sur la sonde à oxygène et son câblage. La valeur ON indique aucun "
    "défaut, OFF indique un problème possible."));
  setTooltip(m_rowClosedLoop,
    tr("Affiche si l'injection est régulée par rétroaction des sondes à oxygène. Sur un véhicule bien chaud, l'état de "
    "boucle doit indiquer boucle fermée dans la plupart des conditions de conduite et de ralenti."));
  setTooltip(m_rowLongTermFuelTrim,
    tr("Affiche la correction carburant actuelle par rétroaction, en pourcentage par rapport à la valeur cartographiée. "
    "Des valeurs élevées (ex. 120%) indiquent une compensation d'un mélange trop pauvre, des valeurs basses (ex. "
    "80%) une compensation d'un mélange trop riche."));
  setTooltip(m_rowShortTermFuelTrim,
    tr("La correction carburant court terme n'est active qu'en boucle fermée. Des valeurs jusqu'à 10% sont normales, "
    "occasionnellement jusqu'à 15%. Des valeurs supérieures à 15% sont anormales."));
  setTooltip(m_rowIgnitionAdvance2,
    tr("Fonction intégrée à l'ECU MEMS pour pallier certaines situations pendant la vie du véhicule (carburant à "
    "faible indice d'octane, usure moteur), en avançant légèrement l'allumage au ralenti."));
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
           data->idle_switch ? tr("Off") : tr("On"));

  setValue(m_rowUk1, QString::number(data->uk1),
           data->uk1 ? tr("On") : tr("Off"));

  setValue(m_rowParkNeutralSwitch,
           QString::number(data->park_neutral_switch),
           data->park_neutral_switch ? tr("On") : tr("Off"));

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
           data->uk3 ? tr("signal") : tr("NO SIGNAL"));
  setValue(m_rowUk4, QString::number(data->uk4), QString::number(data->uk4));
  setValue(m_rowUk5, QString::number(data->uk5), QString::number(data->uk5));

  setValue(m_rowIgnitionSwitch,
           QString::number(data->ignition_switch),
           data->ignition_switch ? tr("On") : tr("Off"));

  setValue(m_rowThrottleAngle,
           QString::number(data->throttle_angle),
           QString::number(data->throttle_angle * 0.6, 'f', 1) + " °");

  setValue(m_rowUk6, QString::number(data->uk6), QString::number(data->uk6));

  setValue(m_rowAirFuelRatio,
           QString::number(data->air_fuel_ratio),
           QString::number(data->air_fuel_ratio / 10.0, 'f', 1) + " AFR");

  setValue(m_rowDtc2,
           QString::number(data->dtc2),
           data->dtc2 != 0 ? tr("DÉFAUTS") : tr("aucun défaut"));

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
           data->lambda_sensor_status == 1 ? tr("Capteur OK") : tr("CAPTEUR DÉFECTUEUX"));

  setValue(m_rowClosedLoop,
           QString::number(data->closed_loop),
           data->closed_loop ? tr("Boucle fermée") : tr("Boucle ouverte"));

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
           data->dtc3 != 255 ? tr("DÉFAUTS") : tr("aucun défaut"));

  setValue(m_rowIdleBasePos,
           QString::number(data->idle_base_pos),
           QString::number(data->idle_base_pos));

  setValue(m_rowUk7,
           QString::number(data->uk7),
           data->uk7 != 255 ? tr("DÉFAUTS") : tr("aucun défaut"));

  setValue(m_rowDtc4,
           QString::number(data->dtc4),
           data->dtc4 != 255 ? tr("DÉFAUTS") : tr("aucun défaut"));

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
           data->dtc5 != 255 ? tr("DÉFAUTS") : tr("aucun défaut"));

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
           data->dtc0 != 0 ? tr("DÉFAUTS") : tr("aucun défaut"));
  setValue(m_rowDtc1,
           QString::number(data->dtc1),
           data->dtc1 != 0 ? tr("DÉFAUTS") : tr("aucun défaut"));
}
