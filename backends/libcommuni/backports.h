#include <QtCore>
#include <iostream>
#include <IrcCommand>
#include <IrcMessage>

namespace CommuniBackport {

bool parseColors(const QString& message, int pos, int* len, int* fg = 0, int* bg = 0)
{
    // fg(,bg)
    *len = 0;
    if (fg)
        *fg = -1;
    if (bg)
        *bg = -1;
    QRegExp rx(QLatin1String("(\\d{1,2})(?:,(\\d{1,2}))?"));
    int idx = rx.indexIn(message, pos);
    if (idx == pos) {
        *len = rx.matchedLength();
        if (fg)
            *fg = rx.cap(1).toInt();
        if (bg) {
            bool ok = false;
            int tmp = rx.cap(2).toInt(&ok);
            if (ok)
                *bg = tmp;
        }
    }
    return *len > 0;
}

/*!
Converts \a text to plain text. This function parses the text and
strips away IRC-style formatting like colors, bold and underline.

\sa toHtml()
*/
void toPlainText(QString& processed)
{

    int pos = 0;
    int len = 0;
    while (pos < processed.size()) {
        switch (processed.at(pos).unicode()) {
            case '\x02': // bold
            case '\x0f': // none
            case '\x13': // strike-through
            case '\x15': // underline
            case '\x16': // inverse
            case '\x1d': // italic
            case '\x1f': // underline
                processed.remove(pos, 1);
                break;

            case '\x03': // color
                if (parseColors(processed, pos + 1, &len))
                    processed.remove(pos, len + 1);
                else
                    processed.remove(pos, 1);
                break;

            default:
                ++pos;
                break;
        }
    }

}

}
