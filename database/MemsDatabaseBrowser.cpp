#include "MemsDatabaseBrowser.h"
#include "MemsReferenceDatabase.h"
#include "../i18n.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDesktopServices>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QXmlStreamReader>

namespace {

QString value(const QVariantMap &row, const char *key)
{
    return row.value(QString::fromLatin1(key)).toString().trimmed();
}

QString escaped(const QVariantMap &row, const char *key)
{
    const QString text=value(row,key);
    return text.isEmpty()?QStringLiteral("—"):text.toHtmlEscaped();
}

QString years(const QVariantMap &row)
{
    const QString from=value(row,"year_from");
    const QString to=value(row,"year_to");
    if(from.isEmpty() && to.isEmpty()) return QStringLiteral("—");
    if(from.isEmpty()) return QStringLiteral("≤ %1").arg(to);
    if(to.isEmpty()) return QStringLiteral("%1 →").arg(from);
    return from==to?from:QStringLiteral("%1–%2").arg(from,to);
}

QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 5px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "h3{color:#f0f3f5;font-size:9.5pt;margin:9px 0 4px 0;}"
        "p{margin:4px 0 7px 0;line-height:1.35}.muted{color:#94a1ab}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;margin:5px 0 8px 0}th{background:#151e25;color:#ff9828;border-bottom:2px solid #ff7a00;text-align:left;padding:5px}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}</style>");
}

QString infoRow(const QString &title,const QString &content)
{
    return QStringLiteral("<tr><th style='width:28%%'>%1</th><td>%2</td></tr>")
        .arg(title.toHtmlEscaped(),content);
}

QFrame *card(QWidget *parent)
{
    QFrame *frame=new QFrame(parent);
    frame->setAttribute(Qt::WA_StyledBackground,true);
    frame->setStyleSheet(QStringLiteral("QFrame{background:#10171d;border:1px solid #29343e;border-radius:5px;}"));
    return frame;
}

QLabel *scopePill(int key,QWidget *parent)
{
    QLabel *label=new QLabel(I18n::text(key),parent);
    label->setProperty("i18nKey",key);
    label->setAlignment(Qt::AlignCenter);
    QFont f=label->font(); f.setBold(true); f.setPointSizeF(8.0); label->setFont(f);
    label->setStyleSheet(QStringLiteral("color:#f0f3f5;background:#172027;border:1px solid #46525c;border-radius:4px;padding:4px 10px;"));
    return label;
}

}

MemsDatabaseBrowser::MemsDatabaseBrowser(QWidget *parent)
    : QWidget(parent),m_database(new MemsReferenceDatabase),m_search(nullptr),m_generation(nullptr),
      m_results(nullptr),m_detail(nullptr),m_resultCount(nullptr),m_title(nullptr),m_subtitle(nullptr),
      m_scopeTitle(nullptr),m_updateTitle(nullptr),m_updateState(nullptr),m_sheetButton(nullptr),m_pdfButton(nullptr)
{
    for(int i=0;i<6;++i){m_metricValues[i]=nullptr;m_metricTitles[i]=nullptr;}
    setObjectName(QStringLiteral("memsDatabaseBrowser"));
    setAttribute(Qt::WA_StyledBackground,true);
    setStyleSheet(QStringLiteral("#memsDatabaseBrowser{background:#090e13;}"));
    buildUi();
    if(m_database->open()){
        updateMetrics();
        refreshResults();
    }else{
        m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='note'>%2</p>")
            .arg(I18n::text(7160).toHtmlEscaped(),I18n::text(7262).toHtmlEscaped()));
    }
}

MemsDatabaseBrowser::~MemsDatabaseBrowser(){delete m_database;}

QLabel *MemsDatabaseBrowser::makeLabel(int key,QWidget *parent)
{
    QLabel *label=new QLabel(I18n::text(key),parent?parent:this);
    label->setProperty("i18nKey",key);
    return label;
}

void MemsDatabaseBrowser::buildUi()
{
    QVBoxLayout *root=new QVBoxLayout(this);
    root->setContentsMargins(10,8,10,8); root->setSpacing(7);

    QFrame *hero=card(this);
    hero->setStyleSheet(QStringLiteral("QFrame{background:#10171d;border:1px solid #36434d;border-left:3px solid #ff7a00;border-radius:5px;}"));
    QVBoxLayout *hv=new QVBoxLayout(hero); hv->setContentsMargins(12,7,12,7); hv->setSpacing(1);
    m_title=makeLabel(7160,hero); QFont tf=m_title->font(); tf.setBold(true); tf.setPointSizeF(10.5); m_title->setFont(tf);
    m_title->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    m_subtitle=makeLabel(7161,hero); m_subtitle->setStyleSheet(QStringLiteral("color:#9aa7b1;background:transparent;border:0;"));
    hv->addWidget(m_title); hv->addWidget(m_subtitle); root->addWidget(hero);

    QFrame *scope=card(this); QHBoxLayout *sh=new QHBoxLayout(scope); sh->setContentsMargins(9,5,9,5); sh->setSpacing(6);
    m_scopeTitle=makeLabel(7162,scope); QFont sf=m_scopeTitle->font(); sf.setBold(true); m_scopeTitle->setFont(sf);
    m_scopeTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;")); sh->addWidget(m_scopeTitle);
    for(int key:{7163,7164,7165,7166}) sh->addWidget(scopePill(key,scope));
    sh->addStretch(1); QLabel *offline=makeLabel(7260,scope); offline->setStyleSheet(QStringLiteral("color:#86d17a;background:transparent;border:0;font-weight:700;")); sh->addWidget(offline);
    root->addWidget(scope);

    QGridLayout *metrics=new QGridLayout; metrics->setContentsMargins(0,0,0,0); metrics->setHorizontalSpacing(6);
    const int metricKeys[]={7168,7169,7170,7171,7172,7173};
    for(int i=0;i<6;++i){
        QFrame *box=card(this); QVBoxLayout *v=new QVBoxLayout(box); v->setContentsMargins(8,5,8,5); v->setSpacing(0);
        m_metricValues[i]=new QLabel(QStringLiteral("—"),box); m_metricValues[i]->setAlignment(Qt::AlignCenter);
        QFont vf=m_metricValues[i]->font(); vf.setBold(true); vf.setPointSizeF(13.5); m_metricValues[i]->setFont(vf);
        m_metricValues[i]->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
        m_metricTitles[i]=makeLabel(metricKeys[i],box); m_metricTitles[i]->setAlignment(Qt::AlignCenter);
        QFont mf=m_metricTitles[i]->font(); mf.setBold(true); mf.setPointSizeF(7.1); m_metricTitles[i]->setFont(mf);
        m_metricTitles[i]->setStyleSheet(QStringLiteral("color:#aeb9c2;background:transparent;border:0;"));
        v->addWidget(m_metricValues[i]); v->addWidget(m_metricTitles[i]); metrics->addWidget(box,0,i); metrics->setColumnStretch(i,1);
    }
    root->addLayout(metrics);

    QFrame *searchBox=card(this); QHBoxLayout *sv=new QHBoxLayout(searchBox); sv->setContentsMargins(9,6,9,6); sv->setSpacing(7);
    QLabel *searchTitle=makeLabel(7216,searchBox); QFont qf=searchTitle->font(); qf.setBold(true); searchTitle->setFont(qf);
    searchTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    m_search=new QLineEdit(searchBox); m_search->setClearButtonEnabled(true); m_search->setMinimumHeight(30);
    m_search->setStyleSheet(QStringLiteral("QLineEdit{background:#0a1117;color:#e7ecef;border:1px solid #34414b;border-radius:4px;padding:4px 8px;}QLineEdit:focus{border-color:#ff7a00;}"));
    m_generation=new QComboBox(searchBox); m_generation->setMinimumHeight(30); m_generation->setMinimumWidth(155);
    m_generation->setStyleSheet(QStringLiteral("QComboBox{background:#0a1117;color:#e7ecef;border:1px solid #34414b;border-radius:4px;padding:4px 8px;}"));
    m_resultCount=new QLabel(searchBox); m_resultCount->setStyleSheet(QStringLiteral("color:#9aa7b1;background:transparent;border:0;"));
    sv->addWidget(searchTitle); sv->addWidget(m_search,1); sv->addWidget(m_generation); sv->addWidget(m_resultCount); root->addWidget(searchBox);

    QSplitter *split=new QSplitter(Qt::Horizontal,this); split->setChildrenCollapsible(false); split->setHandleWidth(7);
    split->setStyleSheet(QStringLiteral("QSplitter::handle{background:#090e13;}"));

    QFrame *listCard=card(split); QVBoxLayout *lv=new QVBoxLayout(listCard); lv->setContentsMargins(7,7,7,7);
    m_results=new QTableWidget(listCard); m_results->setColumnCount(5); m_results->verticalHeader()->setVisible(false);
    m_results->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    m_results->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    m_results->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    m_results->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);
    m_results->horizontalHeader()->setSectionResizeMode(4,QHeaderView::ResizeToContents);
    m_results->setSelectionBehavior(QAbstractItemView::SelectRows); m_results->setSelectionMode(QAbstractItemView::SingleSelection);
    m_results->setEditTriggers(QAbstractItemView::NoEditTriggers); m_results->setAlternatingRowColors(true); m_results->setShowGrid(false);
    m_results->setStyleSheet(QStringLiteral("QTableWidget{background:#0a1117;color:#dce3e8;alternate-background-color:#0e151b;border:1px solid #27323b;outline:0;}QTableWidget::item{padding:4px;border:0;}QTableWidget::item:selected{background:#332414;color:#fff;}QHeaderView::section{background:#141c23;color:#ff9828;border:0;border-right:1px solid #29343e;border-bottom:2px solid #ff7a00;padding:5px;font-weight:700;}"));
    lv->addWidget(m_results,1);

    QFrame *detailCard=card(split); QVBoxLayout *dv=new QVBoxLayout(detailCard); dv->setContentsMargins(7,7,7,7); dv->setSpacing(5);
    QHBoxLayout *tools=new QHBoxLayout; QLabel *detailTitle=makeLabel(7224,detailCard); QFont df=detailTitle->font(); df.setBold(true); detailTitle->setFont(df);
    detailTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    m_sheetButton=new QPushButton(detailCard); m_pdfButton=new QPushButton(detailCard);
    for(QPushButton *b:{m_sheetButton,m_pdfButton}){b->setMinimumHeight(28);b->setStyleSheet(QStringLiteral("QPushButton{background:#182129;color:#e9eef1;border:1px solid #3b4852;border-radius:4px;padding:4px 9px;}QPushButton:hover{border-color:#ff7a00;color:#ffad5c;}"));}
    tools->addWidget(detailTitle); tools->addStretch(1); tools->addWidget(m_sheetButton); tools->addWidget(m_pdfButton); dv->addLayout(tools);
    m_detail=new QTextBrowser(detailCard); m_detail->setStyleSheet(QStringLiteral("QTextBrowser{background:#0a1015;color:#dce3e8;border:1px solid #27323b;border-radius:3px;padding:7px;}")); dv->addWidget(m_detail,1);
    split->addWidget(listCard); split->addWidget(detailCard); split->setStretchFactor(0,3); split->setStretchFactor(1,4); root->addWidget(split,1);

    QFrame *footer=card(this); QHBoxLayout *fv=new QHBoxLayout(footer); fv->setContentsMargins(9,4,9,4);
    m_updateTitle=makeLabel(7255,footer); QFont uf=m_updateTitle->font(); uf.setBold(true); m_updateTitle->setFont(uf); m_updateTitle->setStyleSheet(QStringLiteral("color:#ff9828;background:transparent;border:0;"));
    m_updateState=makeLabel(7256,footer); m_updateState->setStyleSheet(QStringLiteral("color:#8f9aa4;background:transparent;border:0;"));
    QLabel *readOnly=makeLabel(7231,footer); readOnly->setStyleSheet(QStringLiteral("color:#86d17a;background:transparent;border:0;"));
    fv->addWidget(m_updateTitle); fv->addWidget(m_updateState); fv->addStretch(1); fv->addWidget(readOnly); root->addWidget(footer);

    connect(m_search,&QLineEdit::textChanged,this,&MemsDatabaseBrowser::refreshResults);
    connect(m_generation,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&MemsDatabaseBrowser::refreshResults);
    connect(m_results,&QTableWidget::itemSelectionChanged,this,&MemsDatabaseBrowser::showSelectedEcu);
    connect(m_sheetButton,&QPushButton::clicked,this,&MemsDatabaseBrowser::showGenerationSheet);
    connect(m_pdfButton,&QPushButton::clicked,this,&MemsDatabaseBrowser::openGenerationPdf);
    retranslateUi();
}

void MemsDatabaseBrowser::changeEvent(QEvent *event)
{
    if(event && event->type()==QEvent::LanguageChange) retranslateUi();
    QWidget::changeEvent(event);
}

void MemsDatabaseBrowser::retranslateUi()
{
    for(QLabel *label:findChildren<QLabel*>()){
        const QVariant key=label->property("i18nKey"); if(key.isValid()) label->setText(I18n::text(key.toInt()));
    }
    if(m_search) m_search->setPlaceholderText(I18n::text(7217));
    if(m_generation){
        const QString current=m_generation->currentData().toString(); m_generation->blockSignals(true); m_generation->clear();
        m_generation->addItem(I18n::text(7218),QString()); m_generation->addItem(I18n::text(7163),QStringLiteral("1.2"));
        m_generation->addItem(I18n::text(7164),QStringLiteral("1.3")); m_generation->addItem(I18n::text(7165),QStringLiteral("1.6")); m_generation->addItem(I18n::text(7166),QStringLiteral("1.9"));
        const int idx=m_generation->findData(current); m_generation->setCurrentIndex(idx>=0?idx:0); m_generation->blockSignals(false);
    }
    if(m_results){const int keys[]={7219,7220,7221,7222,7223};for(int i=0;i<5;++i){QTableWidgetItem *item=m_results->horizontalHeaderItem(i);if(!item){item=new QTableWidgetItem;m_results->setHorizontalHeaderItem(i,item);}item->setText(I18n::text(keys[i]));}}
    if(m_sheetButton) m_sheetButton->setText(I18n::text(7253)); if(m_pdfButton) m_pdfButton->setText(I18n::text(7261));
    if(m_detail && selectedPartNumber().isEmpty()) m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7224).toHtmlEscaped(),I18n::text(7225).toHtmlEscaped()));
    if(m_database && m_database->isOpen()) refreshResults();
}

void MemsDatabaseBrowser::updateMetrics()
{
    const int values[]={m_database->targetEcuCount(),m_database->targetFileCount(),m_database->targetFitmentCount(),m_database->commandCount(),m_database->dataFieldCount(),m_database->capabilityCount()};
    for(int i=0;i<6;++i) if(m_metricValues[i]) m_metricValues[i]->setText(QString::number(values[i]));
}

QString MemsDatabaseBrowser::generationFilterToken() const{return m_generation?m_generation->currentData().toString():QString();}

void MemsDatabaseBrowser::refreshResults()
{
    if(!m_database||!m_database->isOpen()||!m_results) return;
    const QString previous=selectedPartNumber(); const QVariantList rows=m_database->searchEcus(m_search?m_search->text():QString(),generationFilterToken());
    m_results->blockSignals(true); m_results->setRowCount(rows.size()); int restore=-1;
    for(int r=0;r<rows.size();++r){
        const QVariantMap data=rows.at(r).toMap(); const QString part=value(data,"part_number");
        const QString columns[]={part,value(data,"system_family"),value(data,"injection"),value(data,"models"),years(data)};
        for(int c=0;c<5;++c){QTableWidgetItem *item=new QTableWidgetItem(columns[c]);if(c==0){item->setData(Qt::UserRole,part);QFont f=item->font();f.setBold(true);item->setFont(f);}m_results->setItem(r,c,item);} m_results->setRowHeight(r,27); if(part==previous) restore=r;
    }
    m_results->blockSignals(false); if(m_resultCount) m_resultCount->setText(QStringLiteral("%1: %2").arg(I18n::text(7254),QString::number(rows.size())));
    if(rows.isEmpty()){m_detail->setHtml(htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p>").arg(I18n::text(7224).toHtmlEscaped(),I18n::text(7233).toHtmlEscaped()));return;}
    m_results->selectRow(restore>=0?restore:0); showSelectedEcu();
}

QString MemsDatabaseBrowser::selectedPartNumber() const
{
    if(!m_results||m_results->currentRow()<0) return QString(); QTableWidgetItem *item=m_results->item(m_results->currentRow(),0); return item?item->data(Qt::UserRole).toString():QString();
}

QString MemsDatabaseBrowser::selectedSystemFamily() const
{
    if(!m_results||m_results->currentRow()<0) return QString(); QTableWidgetItem *item=m_results->item(m_results->currentRow(),1); return item?item->text():QString();
}

void MemsDatabaseBrowser::showSelectedEcu()
{
    const QString part=selectedPartNumber(); if(part.isEmpty()) return; m_detail->setHtml(renderEcuHtml(part));
    const QString system=selectedSystemFamily(); const QString xml=m_database->generationXmlPath(system); const QString pdf=m_database->generationPdfPath(system);
    m_sheetButton->setEnabled(!xml.isEmpty()&&QFileInfo::exists(xml)); const bool hasPdf=!pdf.isEmpty()&&QFileInfo::exists(pdf); m_pdfButton->setEnabled(hasPdf); m_pdfButton->setVisible(hasPdf);
}

QString MemsDatabaseBrowser::renderEcuHtml(const QString &partNumber) const
{
    const QVariantMap ecu=m_database->ecu(partNumber); if(ecu.isEmpty()) return htmlStyle(); const QString system=value(ecu,"system_family");
    const QVariantList fit=m_database->fitments(partNumber), files=m_database->files(partNumber), profiles=m_database->protocolProfiles(system), commands=m_database->protocolCommands(system), caps=m_database->protocolCapabilities(system), fields=m_database->protocolDataFields(system), settings=m_database->protocolSettings(system), dtcs=m_database->dtcs(system), actuators=m_database->actuatorSpecs();
    QString h=htmlStyle()+QStringLiteral("<h1>%1</h1><p class='muted'>%2</p><table>").arg(partNumber.toHtmlEscaped(),system.toHtmlEscaped());
    h+=infoRow(I18n::text(7234),escaped(ecu,"brand")); h+=infoRow(I18n::text(7220),system.toHtmlEscaped()); h+=infoRow(I18n::text(7221),escaped(ecu,"injection")); h+=infoRow(I18n::text(7235),escaped(ecu,"makes")); h+=infoRow(I18n::text(7236),escaped(ecu,"models")); h+=infoRow(I18n::text(7223),years(ecu).toHtmlEscaped()); h+=QStringLiteral("</table>");
    const QString notes=value(ecu,"notes"); if(!notes.isEmpty()) h+=QStringLiteral("<div class='note'>%1</div>").arg(notes.toHtmlEscaped());

    h+=QStringLiteral("<h2>%1 (%2)</h2>").arg(I18n::text(7227).toHtmlEscaped(),QString::number(fit.size()));
    if(!fit.isEmpty()){h+=QStringLiteral("<table><tr><th>%1</th><th>%2</th><th>%3</th><th>%4</th><th>%5</th></tr>").arg(I18n::text(7222).toHtmlEscaped(),I18n::text(7237).toHtmlEscaped(),I18n::text(7238).toHtmlEscaped(),I18n::text(7223).toHtmlEscaped(),I18n::text(7239).toHtmlEscaped()); for(const QVariant &v:fit){const QVariantMap f=v.toMap();const QString vehicle=(value(f,"make")+QStringLiteral(" ")+value(f,"model")).trimmed();h+=QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>").arg(vehicle.toHtmlEscaped(),escaped(f,"variant"),escaped(f,"engine"),years(f).toHtmlEscaped(),escaped(f,"market"));}h+=QStringLiteral("</table>");}

    h+=QStringLiteral("<h2>%1 (%2)</h2>").arg(I18n::text(7228).toHtmlEscaped(),QString::number(files.size()));
    if(!files.isEmpty()){h+=QStringLiteral("<table><tr><th>%1</th><th>%2</th></tr>").arg(I18n::text(7247).toHtmlEscaped(),I18n::text(7248).toHtmlEscaped());for(const QVariant &v:files){const QVariantMap f=v.toMap();h+=QStringLiteral("<tr><td>%1</td><td>%2</td></tr>").arg(escaped(f,"filename"),escaped(f,"vehicle"));}h+=QStringLiteral("</table>");}

    h+=QStringLiteral("<h2>%1</h2>").arg(I18n::text(7229).toHtmlEscaped());
    for(const QVariant &v:profiles){const QVariantMap p=v.toMap();h+=QStringLiteral("<table>")+infoRow(I18n::text(7251),escaped(p,"protocol_name"))+infoRow(I18n::text(7263),escaped(p,"physical_layer"))+infoRow(I18n::text(7264),escaped(p,"connector"))+infoRow(I18n::text(7265),escaped(p,"baud_rate"))+infoRow(I18n::text(7266),escaped(p,"wakeup_sequence"))+infoRow(I18n::text(7252),escaped(p,"confidence"))+QStringLiteral("</table>");}

    h+=QStringLiteral("<h2>%1</h2><p>%2: <b>%3</b> &nbsp; %4: <b>%5</b> &nbsp; %6: <b>%7</b></p>").arg(I18n::text(7230).toHtmlEscaped(),I18n::text(7249).toHtmlEscaped(),QString::number(commands.size()),I18n::text(7250).toHtmlEscaped(),QString::number(caps.size()),I18n::text(7267).toHtmlEscaped(),QString::number(fields.size()));
    if(!commands.isEmpty()){h+=QStringLiteral("<h3>%1</h3><table><tr><th>%2</th><th>%3</th><th>%4</th></tr>").arg(I18n::text(7249).toHtmlEscaped(),I18n::text(7284).toHtmlEscaped(),I18n::text(7268).toHtmlEscaped(),I18n::text(7252).toHtmlEscaped());for(const QVariant &v:commands){const QVariantMap c=v.toMap();h+=QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td></tr>").arg(escaped(c,"command_hex"),escaped(c,"name_fr"),escaped(c,"confidence"));}h+=QStringLiteral("</table>");}
    if(!fields.isEmpty()){h+=QStringLiteral("<h3>%1</h3><table><tr><th>%2</th><th>%3</th><th>%4</th></tr>").arg(I18n::text(7267).toHtmlEscaped(),I18n::text(7269).toHtmlEscaped(),I18n::text(7270).toHtmlEscaped(),I18n::text(7271).toHtmlEscaped());for(const QVariant &v:fields){const QVariantMap f=v.toMap();h+=QStringLiteral("<tr><td>%1 @ %2</td><td>%3</td><td>%4 %5</td></tr>").arg(escaped(f,"packet_command"),escaped(f,"byte_offset"),escaped(f,"field_name_fr"),escaped(f,"decoding_formula"),escaped(f,"unit"));}h+=QStringLiteral("</table>");}
    if(!settings.isEmpty()){h+=QStringLiteral("<h3>%1</h3><table><tr><th>%2</th><th>%3</th><th>%4</th></tr>").arg(I18n::text(7272).toHtmlEscaped(),I18n::text(7273).toHtmlEscaped(),I18n::text(7274).toHtmlEscaped(),I18n::text(7275).toHtmlEscaped());for(const QVariant &v:settings){const QVariantMap s=v.toMap();h+=QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3 / %4</td></tr>").arg(escaped(s,"setting_name"),escaped(s,"default_hex"),escaped(s,"increment_command"),escaped(s,"decrement_command"));}h+=QStringLiteral("</table>");}
    if(!dtcs.isEmpty()){h+=QStringLiteral("<h2>%1</h2><table><tr><th>%2</th><th>%3</th><th>%4</th></tr>").arg(I18n::text(7276).toHtmlEscaped(),I18n::text(7277).toHtmlEscaped(),I18n::text(7278).toHtmlEscaped(),I18n::text(7279).toHtmlEscaped());for(const QVariant &v:dtcs){const QVariantMap d=v.toMap();h+=QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td></tr>").arg(escaped(d,"code"),escaped(d,"description"),escaped(d,"possible_causes"));}h+=QStringLiteral("</table>");}
    if(!actuators.isEmpty()){h+=QStringLiteral("<h2>%1</h2><table><tr><th>%2</th><th>%3</th><th>%4</th></tr>").arg(I18n::text(7280).toHtmlEscaped(),I18n::text(7281).toHtmlEscaped(),I18n::text(7282).toHtmlEscaped(),I18n::text(7283).toHtmlEscaped());for(const QVariant &v:actuators){const QVariantMap a=v.toMap();h+=QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td></tr>").arg(escaped(a,"component_name"),escaped(a,"nominal_resistance_ohms"),escaped(a,"testing_notes"));}h+=QStringLiteral("</table>");}
    return h;
}

QString MemsDatabaseBrowser::renderGenerationXml(const QString &path) const
{
    QFile file(path); if(!file.open(QIODevice::ReadOnly|QIODevice::Text)) return htmlStyle()+QStringLiteral("<p class='note'>%1</p>").arg(I18n::text(7232).toHtmlEscaped());
    QXmlStreamReader xml(&file); QString h=htmlStyle(); bool firstRow=true;
    while(!xml.atEnd()){
        xml.readNext();
        if(xml.isEndElement()){if(xml.name()==QStringLiteral("ligne")){h+=QStringLiteral("</tr>");firstRow=false;}else if(xml.name()==QStringLiteral("table"))h+=QStringLiteral("</table>");continue;}
        if(!xml.isStartElement()) continue; const QStringRef name=xml.name();
        if(name==QStringLiteral("titre")) h+=QStringLiteral("<h1>%1</h1>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("sous-titre")) h+=QStringLiteral("<p class='muted'>%1</p>").arg(xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped());
        else if(name==QStringLiteral("section")) h+=QStringLiteral("<h2>%1</h2>").arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());
        else if(name==QStringLiteral("p")){QString t=xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();t.replace(QStringLiteral("\n"),QStringLiteral("<br>"));h+=QStringLiteral("<p>%1</p>").arg(t);}
        else if(name==QStringLiteral("note")){QString t=xml.readElementText(QXmlStreamReader::IncludeChildElements).toHtmlEscaped();t.replace(QStringLiteral("\n"),QStringLiteral("<br>"));h+=QStringLiteral("<div class='note'>%1</div>").arg(t);}
        else if(name==QStringLiteral("table")){firstRow=true;h+=QStringLiteral("<table>");}
        else if(name==QStringLiteral("ligne")) h+=QStringLiteral("<tr>");
        else if(name==QStringLiteral("cellule")){const QString t=xml.readElementText(QXmlStreamReader::IncludeChildElements).trimmed().toHtmlEscaped();const QString tag=firstRow?QStringLiteral("th"):QStringLiteral("td");h+=QStringLiteral("<%1>%2</%1>").arg(tag,t);}
    }
    return h;
}

void MemsDatabaseBrowser::showGenerationSheet()
{
    const QString path=m_database->generationXmlPath(selectedSystemFamily());
    if(path.isEmpty()||!QFileInfo::exists(path)){m_detail->setHtml(htmlStyle()+QStringLiteral("<p class='note'>%1</p>").arg(I18n::text(7232).toHtmlEscaped()));return;}
    m_detail->setHtml(renderGenerationXml(path));
}

void MemsDatabaseBrowser::openGenerationPdf()
{
    const QString path=m_database->generationPdfPath(selectedSystemFamily()); if(!path.isEmpty()&&QFileInfo::exists(path)) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
