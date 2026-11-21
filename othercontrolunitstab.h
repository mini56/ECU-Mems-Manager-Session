#ifndef OTHERCONTROLUNITSTAB_H
#define OTHERCONTROLUNITSTAB_H

// SESSION 186: reserved workspaces for three independent diagnostic families.
// Layout and navigation only. No ECU communication or programming commands.
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include "i18n.h"

class OtherControlUnitsTab final : public QWidget
{
public:
    explicit OtherControlUnitsTab(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("other_ecus_tab"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(0, 0);

        QHBoxLayout *columns = new QHBoxLayout(this);
        columns->setContentsMargins(14, 14, 14, 14);
        columns->setSpacing(12);

        static const char *const panelNames[3] = {
            "otherEcusAirbagRc5", "otherEcusImmobiliser", "otherEcusKeys"
        };
        static const char *const headingNames[3] = {
            "otherEcusAirbagTitle", "otherEcusImmobiliserTitle",
            "otherEcusKeysTitle"
        };

        for (int i = 0; i < 3; ++i)
        {
            QFrame *panel = new QFrame(this);
            panel->setObjectName(QString::fromLatin1(panelNames[i]));
            panel->setAttribute(Qt::WA_StyledBackground, true);
            panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            panel->setMinimumSize(0, 0);

            QVBoxLayout *panelLayout = new QVBoxLayout(panel);
            panelLayout->setContentsMargins(12, 15, 12, 12);
            panelLayout->setSpacing(10);

            QLabel *heading = new QLabel(panel);
            heading->setObjectName(QString::fromLatin1(headingNames[i]));
            heading->setAlignment(Qt::AlignCenter);
            heading->setWordWrap(true);
            QFont font = heading->font();
            font.setPointSizeF(10.0);
            font.setBold(true);
            heading->setFont(font);
            panelLayout->addWidget(heading);

            QFrame *line = new QFrame(panel);
            line->setObjectName(QStringLiteral("otherEcusTitleDivider"));
            line->setFrameShape(QFrame::HLine);
            line->setFixedHeight(1);
            panelLayout->addWidget(line);

            panelLayout->addStretch(1);

            QLabel *placeholder = new QLabel(panel);
            placeholder->setObjectName(QStringLiteral("otherEcusReservedLabel"));
            placeholder->setAlignment(Qt::AlignCenter);
            placeholder->setWordWrap(true);
            panelLayout->addWidget(placeholder);

            panelLayout->addStretch(2);

            m_panels[i] = panel;
            m_headings[i] = heading;
            m_placeholders[i] = placeholder;
            m_dividers[i] = line;
            columns->addWidget(panel, 1);
        }

        retranslate();
        refreshTheme();
    }

    void retranslate()
    {
        for (int i = 0; i < 3; ++i)
        {
            m_headings[i]->setText(I18n::text(16801 + i));
            m_placeholders[i]->setText(I18n::text(16804));
        }
    }

    void refreshTheme()
    {
        const bool dark = qApp && qApp->property("ecuDarkTheme").toBool();
        const QString surface = dark ? QStringLiteral("#10161c")
                                     : QStringLiteral("#ffffff");
        const QString border = dark ? QStringLiteral("#2d3943")
                                    : QStringLiteral("#d5dee7");
        const QString muted = dark ? QStringLiteral("#aebcc5")
                                   : QStringLiteral("#637587");
        for (int i = 0; i < 3; ++i)
        {
            m_panels[i]->setStyleSheet(
                QStringLiteral("QFrame#%1{background:%2;border:1px solid %3;border-radius:6px;}")
                    .arg(m_panels[i]->objectName(), surface, border));
            m_headings[i]->setStyleSheet(
                QStringLiteral("color:%1;background:transparent;border:0;")
                    .arg(dark ? QStringLiteral("#ff9828") : QStringLiteral("#b75500")));
            m_placeholders[i]->setStyleSheet(
                QStringLiteral("color:%1;background:transparent;border:0;").arg(muted));
            m_dividers[i]->setStyleSheet(
                QStringLiteral("background:%1;border:0;").arg(border));
        }
    }

private:
    QFrame *m_panels[3] = {nullptr, nullptr, nullptr};
    QLabel *m_headings[3] = {nullptr, nullptr, nullptr};
    QLabel *m_placeholders[3] = {nullptr, nullptr, nullptr};
    QFrame *m_dividers[3] = {nullptr, nullptr, nullptr};
};

#endif // OTHERCONTROLUNITSTAB_H
