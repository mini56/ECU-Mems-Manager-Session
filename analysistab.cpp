#include "analysistab.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QFontMetrics>
#include <QMap>
#include <QObject>
#include "i18n.h"
#define tr I18n::text

static QString friendlyColumnName(const QString &rawName)
{
  static QMap<QString, QString> names;
  if (names.isEmpty())
  {
    names["80x01-02_engine-rpm"] = I18n::text(6400);
    names["80x03_coolant_temp"] = I18n::text(6401);
    names["80x04_ambient_temp"] = I18n::text(6402);
    names["80x05_intake_air_temp"] = I18n::text(6403);
    names["80x06_fuel_temp"] = I18n::text(6404);
    names["80x07_map_kpa"] = I18n::text(6405);
    names["80x08_battery_voltage"] = I18n::text(6406);
    names["80x09_throttle_pot"] = I18n::text(6407);
    names["80x0A_idle_switch"] = I18n::text(6408);
    names["80x0C_park_neutral_switch"] = I18n::text(6409);
    names["80x0D-0E_fault_codes"] = I18n::text(6410);
    names["80x0F_idle_set_point"] = I18n::text(6411);
    names["80x10_idle_hot"] = I18n::text(6412);
    names["80x12_iac_position"] = I18n::text(6413);
    names["80x13-14_idle_error"] = I18n::text(6414);
    names["80x15_ignition_advance_offset"] = I18n::text(6415);
    names["80x16_ignition_advance"] = I18n::text(6416);
    names["80x17-18_coil_time"] = I18n::text(6417);
    names["80x19_crankshaft_position_sensor"] = I18n::text(6418);
    names["7dx01_ignition_switch"] = I18n::text(6419);
    names["7dx02_throttle_angle"] = I18n::text(6420);
    names["7dx04_air_fuel_ratio"] = I18n::text(6421);
    names["7dx05_dtc2"] = I18n::text(6422);
    names["7dx06_lambda_voltage"] = I18n::text(6423);
    names["7dx07_lambda_sensor_frequency"] = I18n::text(6424);
    names["7dx08_lambda_sensor_dutycycle"] = I18n::text(6425);
    names["7dx09_lambda_sensor_status"] = I18n::text(6426);
    names["7dx0A_closed_loop"] = I18n::text(6427);
    names["7dx0B_long_term_fuel_trim"] = I18n::text(6428);
    names["7dx0C_short_term_fuel_trim"] = I18n::text(6429);
    names["7dx0D_carbon_canister_dutycycle"] = I18n::text(6430);
    names["7dx0E_dtc3"] = I18n::text(6431);
    names["7dx0F_idle_base_pos"] = I18n::text(6432);
    names["7dx11_dtc4"] = I18n::text(6433);
    names["7dx12_ignition_advance2"] = I18n::text(6434);
    names["7dx13_idle_speed_offset"] = I18n::text(6435);
    names["7dx14-15_idle_error_hot_corrected"] = I18n::text(6436);
    names["7dx14-15_raw"] = I18n::text(6437);
    names["7dx16_dtc5"] = I18n::text(6438);
  }
  if (names.contains(rawName)) return names[rawName];
  if (rawName.contains("_uk") || rawName.contains("uk1") || rawName.contains("uk2")) return I18n::text(6439).arg(rawName);
  return rawName;
}

SingleChartWidget::SingleChartWidget(const QString &name, const QColor &color, QWidget *parent)
  : QWidget(parent), m_name(name), m_color(color), m_hasCursor(false), m_cursorX(0)
{
  setMinimumHeight(190); setMaximumHeight(190); setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed); setMouseTracking(true);
}
void SingleChartWidget::setData(const QVector<double>&time,const QVector<double>&values){m_time=time;m_values=values;update();}
void SingleChartWidget::mouseMoveEvent(QMouseEvent*event){m_hasCursor=true;m_cursorX=event->pos().x();update();}
void SingleChartWidget::leaveEvent(QEvent*event){Q_UNUSED(event);m_hasCursor=false;update();}
void SingleChartWidget::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event); QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(rect(),QColor("#15181c")); painter.setPen(QColor("#e7ebee")); QFont titleFont=painter.font();titleFont.setBold(true);titleFont.setPointSize(9);painter.setFont(titleFont);painter.drawText(QRect(0,4,width(),18),Qt::AlignHCenter,m_name);
  const int leftMargin=55,rightMargin=10,topMargin=26,bottomMargin=20; QRect plotRect(leftMargin,topMargin,width()-leftMargin-rightMargin,height()-topMargin-bottomMargin);
  painter.setPen(QPen(QColor("#343941"),1)); painter.setBrush(QColor("#101317")); painter.drawRect(plotRect);
  if(m_time.count()<2||m_values.count()<2){painter.setPen(QColor("#737b83"));painter.drawText(plotRect,Qt::AlignCenter,I18n::text(6440));return;}
  double tMin=m_time.first(),tMax=m_time.last(),tSpan=(tMax-tMin)>0.0001?(tMax-tMin):1.0; double vMin=m_values[0],vMax=m_values[0]; for(int i=1;i<m_values.count();i++){if(m_values[i]<vMin)vMin=m_values[i];if(m_values[i]>vMax)vMax=m_values[i];} if(vMax-vMin<0.0001){vMax+=1;vMin-=1;} double margin=(vMax-vMin)*.08;vMax+=margin;vMin-=margin;double vSpan=vMax-vMin;
  QFont axisFont=painter.font();axisFont.setBold(false);axisFont.setPointSize(7);painter.setFont(axisFont); for(int i=0;i<=4;i++){int y=plotRect.top()+(plotRect.height()*i)/4;painter.setPen(QColor("#292e34"));painter.drawLine(plotRect.left(),y,plotRect.right(),y);double val=vMax-(vSpan*i)/4.;painter.setPen(QColor("#8b939b"));painter.drawText(QRect(0,y-8,leftMargin-6,16),Qt::AlignRight|Qt::AlignVCenter,QString::number(val,'f',val<10?1:0));}
  painter.setPen(QPen(m_color,2));QPolygonF poly;int n=qMin(m_time.count(),m_values.count());for(int i=0;i<n;i++){double x=(m_time[i]-tMin)/tSpan,y=(m_values[i]-vMin)/vSpan;poly<<QPointF(plotRect.left()+x*plotRect.width(),plotRect.bottom()-y*plotRect.height());}painter.drawPolyline(poly);
  painter.setPen(QColor("#8b939b"));for(int i=0;i<=5;i++){double t=tMin+(tSpan*i)/5.;int x=plotRect.left()+(plotRect.width()*i)/5;painter.drawText(QRect(x-30,plotRect.bottom()+2,60,16),Qt::AlignCenter,QString("%1 s").arg(t-tMin,0,'f',0));}
  if(m_hasCursor&&m_cursorX>=plotRect.left()&&m_cursorX<=plotRect.right()){painter.setPen(QPen(QColor("#c8cdd2"),1,Qt::DashLine));painter.drawLine(m_cursorX,plotRect.top(),m_cursorX,plotRect.bottom());double xf=double(m_cursorX-plotRect.left())/plotRect.width(),tc=tMin+xf*tSpan;int idx=0;double best=1e18;for(int i=0;i<m_time.count();i++){double d=qAbs(m_time[i]-tc);if(d<best){best=d;idx=i;}}if(idx<m_values.count()){QString label=QString::number(m_values[idx],'f',1);QFontMetrics fm(axisFont);int tw=fm.horizontalAdvance(label)+10,bx=m_cursorX+6;if(bx+tw>plotRect.right())bx=m_cursorX-6-tw;painter.setPen(Qt::NoPen);painter.setBrush(QColor(20,24,28,235));painter.drawRect(bx,plotRect.top()+4,tw,16);painter.setPen(m_color);painter.drawText(QRect(bx+5,plotRect.top()+4,tw,16),Qt::AlignVCenter|Qt::AlignLeft,label);}}
}

ChartWidget::ChartWidget(QWidget *parent):QWidget(parent),m_hasCursor(false),m_cursorX(0){setMinimumHeight(150);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);setMouseTracking(true);}
void ChartWidget::setData(const QVector<double>&time,const QVector<QVector<double> >&series,const QStringList&names,const QVector<QColor>&colors){m_time=time;m_series=series;m_names=names;m_colors=colors;m_visible=QVector<bool>(names.count(),false);update();}
void ChartWidget::setVisible(int index,bool vis){if(index>=0&&index<m_visible.count()){m_visible[index]=vis;update();}}
void ChartWidget::clearData(){m_time.clear();m_series.clear();m_names.clear();m_colors.clear();m_visible.clear();update();}
void ChartWidget::mouseMoveEvent(QMouseEvent*event){m_hasCursor=true;m_cursorX=event->pos().x();update();}
void ChartWidget::leaveEvent(QEvent*event){Q_UNUSED(event);m_hasCursor=false;update();}
void ChartWidget::paintEvent(QPaintEvent *event)
{
 Q_UNUSED(event);QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);painter.fillRect(rect(),QColor("#101317"));const int l=8,r=8,t=10,b=26;QRect pr(l,t,width()-l-r,height()-t-b);painter.setPen(QPen(QColor("#292e34"),1));for(int i=0;i<=4;i++){int y=pr.top()+pr.height()*i/4;painter.drawLine(pr.left(),y,pr.right(),y);}for(int i=0;i<=10;i++){int x=pr.left()+pr.width()*i/10;painter.drawLine(x,pr.top(),x,pr.bottom());}
 if(m_time.count()<2){painter.setPen(QColor("#8b939b"));painter.drawText(rect(),Qt::AlignCenter,I18n::text(6440));return;}double tMin=m_time.first(),tMax=m_time.last(),span=(tMax-tMin)>.0001?tMax-tMin:1.;int any=0;for(int s=0;s<m_series.count();s++){if(s>=m_visible.count()||!m_visible[s])continue;any++;const QVector<double>&v=m_series[s];if(v.isEmpty())continue;double mn=v[0],mx=v[0];for(int i=1;i<v.count();i++){mn=qMin(mn,v[i]);mx=qMax(mx,v[i]);}double vs=(mx-mn)>.0001?mx-mn:1.;painter.setPen(QPen(m_colors[s%m_colors.count()],2));QPolygonF poly;int n=qMin(m_time.count(),v.count());for(int i=0;i<n;i++){double xf=(m_time[i]-tMin)/span,yf=(v[i]-mn)/vs;poly<<QPointF(pr.left()+xf*pr.width(),pr.bottom()-yf*pr.height());}painter.drawPolyline(poly);}if(!any){painter.setPen(QColor("#8b939b"));painter.drawText(rect(),Qt::AlignCenter,I18n::text(6441));}painter.setPen(QColor("#8b939b"));for(int i=0;i<=5;i++){double tv=tMin+span*i/5.;int x=pr.left()+pr.width()*i/5;painter.drawText(QRect(x-30,pr.bottom()+4,60,18),Qt::AlignCenter,QString("%1 s").arg(tv-tMin,0,'f',0));}
 if(m_hasCursor&&m_cursorX>=pr.left()&&m_cursorX<=pr.right()&&any>0){painter.setPen(QPen(QColor("#e6e8ee"),1,Qt::DashLine));painter.drawLine(m_cursorX,pr.top(),m_cursorX,pr.bottom());double xf=double(m_cursorX-pr.left())/pr.width(),tc=tMin+xf*span;int idx=0;double best=1e18;for(int i=0;i<m_time.count();i++){double d=qAbs(m_time[i]-tc);if(d<best){best=d;idx=i;}}int ly=pr.top()+6;QFont f=painter.font();f.setPointSize(8);painter.setFont(f);for(int s=0;s<m_series.count();s++){if(s>=m_visible.count()||!m_visible[s]||idx>=m_series[s].count())continue;QString label=QString("%1 : %2").arg(m_names[s]).arg(m_series[s][idx],0,'f',1);QFontMetrics fm(f);int tw=fm.horizontalAdvance(label)+10,bx=m_cursorX+8;if(bx+tw>pr.right())bx=m_cursorX-8-tw;painter.setPen(Qt::NoPen);painter.setBrush(QColor(14,20,32,220));painter.drawRect(bx,ly,tw,16);painter.setPen(m_colors[s%m_colors.count()]);painter.drawText(QRect(bx+5,ly,tw,16),Qt::AlignVCenter|Qt::AlignLeft,label);ly+=18;}}
}

AnalysisTab::AnalysisTab(QWidget *parent):QWidget(parent),m_overlayMode(false)
{
 m_colors<<QColor("#4fc3f7")<<QColor("#ff7043")<<QColor("#66bb6a")<<QColor("#ffca28")<<QColor("#ba68c8")<<QColor("#26c6da")<<QColor("#ef5350")<<QColor("#9ccc65")<<QColor("#ec407a")<<QColor("#7e57c2")<<QColor("#8d6e63")<<QColor("#5c6bc0");
 setStyleSheet("AnalysisTab{background:#121519;color:#e7ebee;} QWidget{color:#e7ebee;} QPushButton{background:#20242a;color:#e7ebee;border:1px solid #3a4047;border-radius:5px;padding:7px 10px;} QPushButton:hover{border-color:#ff8a1c;} QPushButton:checked{background:#ff8a1c;color:#111;border-color:#ff8a1c;} QScrollArea{background:#15181c;border:1px solid #343941;border-radius:6px;} QScrollArea>QWidget>QWidget{background:#15181c;}");
 QVBoxLayout*root=new QVBoxLayout(this);root->setContentsMargins(10,10,10,10);QHBoxLayout*main=new QHBoxLayout();main->setSpacing(10);root->addLayout(main,1);QWidget*left=new QWidget(this);left->setObjectName("analysisLeftPanel");left->setStyleSheet("#analysisLeftPanel{background:#1b1f24;border:1px solid #343941;border-radius:8px;}");left->setMaximumWidth(320);left->setMinimumWidth(300);QVBoxLayout*ll=new QVBoxLayout(left);ll->setContentsMargins(12,12,12,12);
 m_loadButton=new QPushButton(I18n::text(6442),left);connect(m_loadButton,SIGNAL(clicked()),this,SLOT(onLoadFileClicked()));ll->addWidget(m_loadButton);m_fileLabel=new QLabel(I18n::text(6443),left);m_fileLabel->setWordWrap(true);m_fileLabel->setStyleSheet("color:#9aa1a8;font-style:italic;");ll->addWidget(m_fileLabel);QHBoxLayout*sr=new QHBoxLayout();m_selectAllButton=new QPushButton(I18n::text(6444),left);m_selectNoneButton=new QPushButton(I18n::text(6445),left);connect(m_selectAllButton,SIGNAL(clicked()),this,SLOT(onSelectAllClicked()));connect(m_selectNoneButton,SIGNAL(clicked()),this,SLOT(onSelectNoneClicked()));sr->addWidget(m_selectAllButton);sr->addWidget(m_selectNoneButton);ll->addLayout(sr);m_overlayButton=new QPushButton(I18n::text(6446),left);m_overlayButton->setCheckable(true);m_overlayButton->setMinimumHeight(44);connect(m_overlayButton,SIGNAL(toggled(bool)),this,SLOT(onOverlayToggled(bool)));ll->addWidget(m_overlayButton);QLabel*vl=new QLabel(I18n::text(6447),left);vl->setStyleSheet("font-weight:600;margin-top:6px;color:#ff9b3d;");ll->addWidget(vl);m_checkboxScrollArea=new QScrollArea(left);m_checkboxScrollArea->setWidgetResizable(true);m_checkboxContainer=new QWidget();m_checkboxLayout=new QVBoxLayout(m_checkboxContainer);m_checkboxLayout->addStretch();m_checkboxScrollArea->setWidget(m_checkboxContainer);ll->addWidget(m_checkboxScrollArea,1);main->addWidget(left);
 m_stackScrollArea=new QScrollArea(this);m_stackScrollArea->setWidgetResizable(true);m_stackContainer=new QWidget();m_stackLayout=new QVBoxLayout(m_stackContainer);m_stackLayout->setSpacing(8);m_stackLayout->addStretch();m_stackScrollArea->setWidget(m_stackContainer);main->addWidget(m_stackScrollArea,1);m_overlayChart=new ChartWidget(this);m_overlayChart->hide();main->addWidget(m_overlayChart,1);
}
void AnalysisTab::onLoadFileClicked(){QString path=QFileDialog::getOpenFileName(this,I18n::text(6448),"logs",I18n::text(6449));if(!path.isEmpty())loadFile(path);}
void AnalysisTab::loadFile(const QString&path){parseCsv(path);}
void AnalysisTab::parseCsv(const QString&path){QFile file(path);if(!file.open(QFile::ReadOnly|QFile::Text)){QMessageBox::warning(this,I18n::text(6450),I18n::text(6451).arg(path),QMessageBox::Ok);return;}QTextStream stream(&file);if(!stream.atEnd())stream.readLine();if(stream.atEnd()){QMessageBox::warning(this,I18n::text(6452),I18n::text(6453),QMessageBox::Ok);return;}QStringList headers=stream.readLine().split(",");if(!headers.isEmpty()&&headers[0].startsWith("#"))headers[0]=headers[0].mid(1);int cc=headers.count()-1;if(cc<=0){QMessageBox::warning(this,I18n::text(6454),I18n::text(6455),QMessageBox::Ok);return;}m_columnNames.clear();for(int i=1;i<headers.count();i++)m_columnNames<<friendlyColumnName(headers[i].trimmed());m_time.clear();m_columns.clear();m_columns.resize(cc);QTime first;int lines=0;while(!stream.atEnd()){QString line=stream.readLine();if(line.trimmed().isEmpty())continue;QStringList f=line.split(",");if(f.count()<2)continue;QTime tm=QTime::fromString(f[0],"hh:mm:ss");double sec;if(tm.isValid()){if(!first.isValid())first=tm;sec=first.secsTo(tm);if(sec<0)sec+=86400;}else sec=lines;m_time.append(sec);for(int c=0;c<cc;c++){double v=0;if(c+1<f.count())v=f[c+1].trimmed().toDouble();m_columns[c].append(v);}lines++;}file.close();if(!lines){QMessageBox::warning(this,I18n::text(6456),I18n::text(6457),QMessageBox::Ok);return;}m_fileLabel->setText(QFileInfo(path).fileName()+" ("+QString::number(lines)+" points)");m_overlayChart->setData(m_time,m_columns,m_columnNames,m_colors);rebuildCheckboxes();}
void AnalysisTab::rebuildCheckboxes(){for(QCheckBox*cb:m_checkboxes){m_checkboxLayout->removeWidget(cb);delete cb;}m_checkboxes.clear();qDeleteAll(m_stackedCharts);m_stackedCharts.clear();QLayoutItem*stretch=m_checkboxLayout->takeAt(m_checkboxLayout->count()-1);for(int i=0;i<m_columnNames.count();i++){QCheckBox*cb=new QCheckBox(m_columnNames[i],m_checkboxContainer);QColor c=m_colors[i%m_colors.count()];cb->setStyleSheet(QString("QCheckBox{padding:4px 0;color:#dfe4e8;} QCheckBox::indicator{width:14px;height:14px;border:2px solid %1;border-radius:3px;background:#101317;} QCheckBox::indicator:checked{background:%1;}").arg(c.name()));if(m_columnNames[i].contains(I18n::text(6458))||m_columnNames[i].contains(I18n::text(6459))||m_columnNames[i].contains(I18n::text(6460)))cb->setChecked(true);connect(cb,SIGNAL(toggled(bool)),this,SLOT(onCheckboxToggled(bool)));m_checkboxLayout->addWidget(cb);m_checkboxes.append(cb);}if(stretch)m_checkboxLayout->addItem(stretch);else m_checkboxLayout->addStretch();updateChartVisibility();}
void AnalysisTab::onCheckboxToggled(bool checked){QCheckBox*cb=qobject_cast<QCheckBox*>(sender());if(!cb)return;int i=m_checkboxes.indexOf(cb);if(i<0)return;if(m_overlayMode)m_overlayChart->setVisible(i,checked);else{if(checked)addStackedChart(i);else removeStackedChart(i);}}
void AnalysisTab::addStackedChart(int i){if(m_stackedCharts.contains(i)||i<0||i>=m_columns.count())return;SingleChartWidget*c=new SingleChartWidget(m_columnNames[i],m_colors[i%m_colors.count()],m_stackContainer);c->setData(m_time,m_columns[i]);m_stackLayout->insertWidget(m_stackLayout->count()-1,c);m_stackedCharts[i]=c;}
void AnalysisTab::removeStackedChart(int i){if(!m_stackedCharts.contains(i))return;SingleChartWidget*c=m_stackedCharts.take(i);m_stackLayout->removeWidget(c);delete c;}
void AnalysisTab::onSelectAllClicked(){for(QCheckBox*cb:m_checkboxes)cb->setChecked(true);}
void AnalysisTab::onSelectNoneClicked(){for(QCheckBox*cb:m_checkboxes)cb->setChecked(false);}
void AnalysisTab::updateChartVisibility(){for(int i=0;i<m_checkboxes.count();i++){bool c=m_checkboxes[i]->isChecked();m_overlayChart->setVisible(i,c);if(!m_overlayMode){if(c)addStackedChart(i);else removeStackedChart(i);}}}
void AnalysisTab::onOverlayToggled(bool checked){m_overlayMode=checked;if(checked){m_overlayButton->setText(I18n::text(6461));m_stackScrollArea->hide();m_overlayChart->show();rebuildOverlayChart();}else{m_overlayButton->setText(I18n::text(6462));m_overlayChart->hide();m_stackScrollArea->show();updateChartVisibility();}}
void AnalysisTab::rebuildOverlayChart(){for(int i=0;i<m_checkboxes.count();i++)m_overlayChart->setVisible(i,m_checkboxes[i]->isChecked());}
