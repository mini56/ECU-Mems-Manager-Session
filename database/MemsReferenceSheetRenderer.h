#ifndef MEMSREFERENCESHEETRENDERER_H
#define MEMSREFERENCESHEETRENDERER_H

#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>

namespace MemsReferenceSheetRenderer {

inline QString htmlStyle()
{
    return QStringLiteral(
        "<style>body{background:#0a1015;color:#dce3e8;font-family:'Segoe UI',Arial,sans-serif;font-size:9pt;}"
        "h1{color:#ff9828;font-size:16pt;margin:0 0 5px 0;}"
        "h2{color:#ff9828;font-size:10.5pt;border-bottom:1px solid #34414b;padding-bottom:4px;margin-top:14px;}"
        "p{margin:4px 0 7px 0;line-height:1.35}.muted{color:#94a1ab}.note{background:#15100b;border:1px solid #60401f;color:#ffd0a0;padding:7px;}"
        "table{border-collapse:collapse;width:100%;margin:5px 0 8px 0}th{background:#151e25;color:#ff9828;border-bottom:2px solid #ff7a00;text-align:left;padding:5px}"
        "td{border-bottom:1px solid #26323b;padding:5px;vertical-align:top}.wire{font-size:14pt;line-height:10px;margin-right:2px}</style>");
}

struct ElementContent
{
    QString text;
    QStringList fills;
};

inline bool usefulFill(const QString &fill)
{
    const QString value = fill.trimmed().toLower();
    return !value.isEmpty() && value != QStringLiteral("none") && value != QStringLiteral("transparent");
}

inline ElementContent readElementContent(QXmlStreamReader &xml)
{
    ElementContent result;
    int depth = 1;
    while (!xml.atEnd() && depth > 0) {
        xml.readNext();
        if (xml.isStartElement()) {
            ++depth;
            const QString name = xml.name().toString().toLower();
            if (name == QStringLiteral("br"))
                result.text += QLatin1Char('\n');
            const QString fill = xml.attributes().value(QStringLiteral("fill")).toString();
            if (usefulFill(fill) && !result.fills.contains(fill, Qt::CaseInsensitive) && result.fills.size() < 2)
                result.fills.append(fill);
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            const QString value = xml.text().toString().trimmed();
            if (!value.isEmpty()) {
                if (!result.text.isEmpty() && !result.text.endsWith(QLatin1Char('\n')))
                    result.text += QLatin1Char(' ');
                result.text += value;
            }
        } else if (xml.isEndElement()) {
            --depth;
        }
    }
    return result;
}

inline QString renderElementContent(const ElementContent &content)
{
    QString html;
    for (const QString &fill : content.fills) {
        html += QStringLiteral("<span class='wire' style='color:%1'>&#9632;</span>")
                    .arg(fill.toHtmlEscaped());
    }
    QString text = content.text.simplified().toHtmlEscaped();
    if (!text.isEmpty()) {
        if (!html.isEmpty())
            html += QStringLiteral("&nbsp;");
        html += text;
    }
    return html.isEmpty() ? QStringLiteral("—") : html;
}

inline QString renderFile(const QString &path, const QString &errorText)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return htmlStyle() + QStringLiteral("<p class='note'>%1</p>").arg(errorText.toHtmlEscaped());

    QXmlStreamReader xml(&file);
    QString html = htmlStyle();
    bool firstRow = true;
    bool inTable = false;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement()) {
            const QString name = xml.name().toString().toLower();
            if (name == QStringLiteral("ligne")) {
                html += QStringLiteral("</tr>");
                firstRow = false;
            } else if (name == QStringLiteral("table")) {
                html += QStringLiteral("</table>");
                inTable = false;
            }
            continue;
        }
        if (!xml.isStartElement())
            continue;

        const QString name = xml.name().toString().toLower();
        if (name == QStringLiteral("titre")) {
            html += QStringLiteral("<h1>%1</h1>")
                        .arg(readElementContent(xml).text.simplified().toHtmlEscaped());
        } else if (name == QStringLiteral("sous-titre")) {
            html += QStringLiteral("<p class='muted'>%1</p>")
                        .arg(readElementContent(xml).text.simplified().toHtmlEscaped());
        } else if (name == QStringLiteral("section")) {
            html += QStringLiteral("<h2>%1</h2>")
                        .arg(xml.attributes().value(QStringLiteral("titre")).toString().toHtmlEscaped());
        } else if (name == QStringLiteral("p")) {
            ElementContent content = readElementContent(xml);
            QString text = content.text.toHtmlEscaped();
            text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
            html += QStringLiteral("<p>%1</p>").arg(text);
        } else if (name == QStringLiteral("note")) {
            ElementContent content = readElementContent(xml);
            QString text = content.text.toHtmlEscaped();
            text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
            html += QStringLiteral("<div class='note'>%1</div>").arg(text);
        } else if (name == QStringLiteral("table")) {
            firstRow = true;
            inTable = true;
            html += QStringLiteral("<table>");
        } else if (name == QStringLiteral("ligne")) {
            if (!inTable) {
                firstRow = true;
                inTable = true;
                html += QStringLiteral("<table>");
            }
            html += QStringLiteral("<tr>");
        } else if (name == QStringLiteral("cellule")
                   || name == QStringLiteral("broche")
                   || name == QStringLiteral("fonction")
                   || name == QStringLiteral("couleur")) {
            const ElementContent content = readElementContent(xml);
            const QString tag = firstRow ? QStringLiteral("th") : QStringLiteral("td");
            html += QStringLiteral("<%1>%2</%1>").arg(tag, renderElementContent(content));
        }
    }

    if (inTable)
        html += QStringLiteral("</table>");
    if (xml.hasError())
        html += QStringLiteral("<div class='note'>%1</div>").arg(errorText.toHtmlEscaped());
    return html;
}

} // namespace MemsReferenceSheetRenderer

#endif // MEMSREFERENCESHEETRENDERER_H
