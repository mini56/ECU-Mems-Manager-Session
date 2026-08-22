#ifndef ANALYSISTAB_H
#define ANALYSISTAB_H

#include <QWidget>
#include <QVector>
#include <QStringList>
#include <QColor>
#include <QCheckBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QMap>
#include <QElapsedTimer>
#include "rosco.h"

class QButtonGroup;

class SingleChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SingleChartWidget(const QString &name, const QColor &color, QWidget *parent = 0);
    void setData(const QVector<double> &time, const QVector<double> &values);
    const QString &chartName() const { return m_name; }
    const QColor &chartColor() const { return m_color; }
    const QVector<double> &chartTime() const { return m_time; }
    const QVector<double> &chartValues() const { return m_values; }
    bool chartHasCursor() const { return m_hasCursor; }
    int chartCursorX() const { return m_cursorX; }
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    QString m_name;
    QColor m_color;
    QVector<double> m_time;
    QVector<double> m_values;
    bool m_hasCursor;
    int m_cursorX;
};

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = 0);
    void setData(const QVector<double> &time,
                 const QVector<QVector<double> > &series,
                 const QStringList &names,
                 const QVector<QColor> &colors);
    void setVisible(int index, bool visible);
    void clearData();
protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    QVector<double> m_time;
    QVector<QVector<double> > m_series;
    QStringList m_names;
    QVector<QColor> m_colors;
    QVector<bool> m_visible;
    bool m_hasCursor;
    int m_cursorX;
};

class AnalysisTab : public QWidget
{
    Q_OBJECT
public:
    enum LiveSource {
        FileSource,
        DiagnosticLiveSource,
        InjectionLiveSource
    };

    explicit AnalysisTab(QWidget *parent = 0);
    void loadFile(const QString &path);

    bool diagnosticLiveSelected() const { return m_liveSource == DiagnosticLiveSource; }
    bool injectionLiveSelected() const { return m_liveSource == InjectionLiveSource; }
    bool fileSourceSelected() const { return m_liveSource == FileSource; }
    void selectDiagnosticLive(bool notify = true);
    void selectInjectionLive(bool notify = true);
    void setDiagnosticLiveAvailable(bool available);
    void setInjectionRamReadAvailable(bool available, const QString &reason = QString());
    void appendDiagnosticData(const mems_data *data);
    void appendInjectionSample(double finalMs, double baseMs, double transientMs,
                               quint16 baseRaw, quint16 transientRaw, quint8 transientCounter);

signals:
    void liveSourceRequested(bool injectionMode);

private slots:
    void onLoadFileClicked();
    void onCheckboxToggled(bool checked);
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void onOverlayToggled(bool checked);
    void onDiagnosticLiveClicked();
    void onInjectionLiveClicked();

private:
    void parseCsv(const QString &path);
    void rebuildCheckboxes();
    void updateChartVisibility();
    void addStackedChart(int index);
    void removeStackedChart(int index);
    void rebuildOverlayChart();
    void prepareDiagnosticLive();
    void prepareInjectionLive();
    void appendLiveRow(const QVector<double> &values);
    void refreshLiveCharts();
    void setLiveButtons(bool diagnostic, bool injection);

    QLabel *m_fileLabel;
    QPushButton *m_loadButton;
    QPushButton *m_selectAllButton;
    QPushButton *m_selectNoneButton;
    QPushButton *m_overlayButton;
    QPushButton *m_diagnosticLiveButton;
    QPushButton *m_injectionLiveButton;
    QButtonGroup *m_liveButtonGroup;

    QScrollArea *m_checkboxScrollArea;
    QWidget *m_checkboxContainer;
    QVBoxLayout *m_checkboxLayout;

    QScrollArea *m_stackScrollArea;
    QWidget *m_stackContainer;
    QVBoxLayout *m_stackLayout;
    QMap<int, SingleChartWidget*> m_stackedCharts;

    ChartWidget *m_overlayChart;
    bool m_overlayMode;
    LiveSource m_liveSource;
    QElapsedTimer m_liveClock;

    QVector<double> m_time;
    QVector<QVector<double> > m_columns;
    QStringList m_columnNames;
    QVector<QCheckBox*> m_checkboxes;
    QVector<QColor> m_colors;
};

#endif // ANALYSISTAB_H
