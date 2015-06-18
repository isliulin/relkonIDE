#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include "leftverticalwidget.h"


/* определение и сохранение номеров текстовых блоков и их координат по вертикали */
void CodeEditor::scanBlocksNums()
{
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    numList.clear();
    while(block.isValid() && top <= viewport()->rect().bottom()) {
        if(block.isVisible() && bottom >= viewport()->rect().top()) {
            numList+=blockNumber;   // номер блока
            numList+=top;   // y координата
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
    lNumbers->updateData("lineNums",numList); // lineNums - идентификатор считанных настроек
    scanFolding();
}

CodeEditor::CodeEditor(QWidget *parent): QPlainTextEdit(parent)
{

    lNumbers = new LeftVerticalWidget(this);
    connect(lNumbers,SIGNAL(sendValue(QString,int)),this,SLOT(getCmdFromChildWidget(QString,int)));

    setViewportMargins(lNumbers->width(),0,0,0);


    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(this, SIGNAL(textChanged()), this , SLOT(scanFolding()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    scanBlocksNums();
    scanFolding();
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
}

/* возвращает необходимую ширину для виджета с номерами текстовых блоков */
int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1,blockCount());
    while(max>=10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * (digits+2);
    return space;
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    scanBlocksNums();
    QRect cr = contentsRect();
    lNumbers->setGeometry(QRect(cr.left(),cr.top(),lNumbers->width(),cr.height()));
}

void CodeEditor::paintEvent(QPaintEvent *event)
{
    QPlainTextEdit::paintEvent(event);
}

void CodeEditor::mousePressEvent(QMouseEvent *event)
{
    QPlainTextEdit::mousePressEvent(event);
    QPlainTextEdit::ensureCursorVisible();
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    scanBlocksNums();
    lNumbers->update();
}

/* подсветка строки с курсором */
void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if(!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::blue).lighter(190);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection,true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    Q_UNUSED(rect);
    Q_UNUSED(dy);
    scanBlocksNums();
    lNumbers->update();
}

/* поиск и сохранение фрагментов кода, допускающих сворачивание разворачивание */
void CodeEditor::scanFolding(void)
{
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    foldList.clear();
    while(block.isValid() && top <= viewport()->rect().bottom()) {
        if(block.isVisible() && bottom >= viewport()->rect().top()) {
            QRegExp rExp1("\\s*#PROCESS\\s+\\d+\\b");
            QRegExp rExp2("\\s*#DATA\\b");
            if(block.text().contains(rExp1) || block.text().contains(rExp2)) {
                if(block.next().isValid()) {
                    foldList += blockNumber;    // номер текстового блока
                    foldList += top;    // y координата
                    if(block.next().isVisible()) {
                        foldList += 1;  // блок развёрнут
                    }
                    else {
                        foldList += 0;  // блок свёрнут
                    }
                }
            }
            else {
                /* раскрытие свёрнутых блоков в случае повреждения заголовка */
                if(block.next().isValid()) {
                    if(!block.next().isVisible()) {
                        toggleFolding(block);
                    }
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
    lNumbers->updateData("folding",foldList); // folding - идентификатор настроек
    lNumbers->update();
}

/* обработка сигнала, полученного от дочернего виджета */
void CodeEditor::getCmdFromChildWidget(QString code, int value)
{
    if(!code.contains("folding")) return; // сигнал на сворачивание/разворачивание блока текста
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    foldList.clear();
    while(block.isValid() && top <= viewport()->rect().bottom()) {
        if(block.isVisible() && bottom >= viewport()->rect().top()) {
            if(blockNumber==value) {
                if(block.next().isValid()) {
                    toggleFolding(block);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
    scanBlocksNums();
}

/* сворачивание/разворачивание текстового блока */
void CodeEditor::toggleFolding(QTextBlock &block)
{
    int startNum = block.blockNumber();
    int endNum = block.blockNumber();

    QTextBlock tmpBl = block.next();
    if(tmpBl.isValid()) {
        if(!tmpBl.isVisible()) {
            while(tmpBl.isValid()) {
                if(tmpBl.isVisible()) break;
                tmpBl.setVisible(true);
                tmpBl.setLineCount(qMax(1,block.next().layout()->lineCount()));
                tmpBl = tmpBl.next();
            }
        }
        else {
            while(tmpBl.isValid()) {
                QRegExp rExp1("\\s*#PROCESS\\s+\\d+\\b");
                QRegExp rExp2("\\s*#INIT\\b");
                if(tmpBl.text().contains(rExp1) || tmpBl.text().contains(rExp2)) break;
                QString txt = tmpBl.text();
                txt.remove(' ');
                txt.remove('\t');

                if(txt.length() && (txt.left(2) != "//")) endNum = tmpBl.blockNumber();
                tmpBl = tmpBl.next();
            }
            if(endNum>startNum) {
                QTextCursor tmpCursor = textCursor();
                tmpCursor.setVisualNavigation(true);
                tmpCursor.setPosition(document()->findBlockByNumber(startNum).position());
                setTextCursor(tmpCursor);

                startNum++;
                for(int i=startNum;i<=endNum;i++) {
                    tmpBl = document()->findBlockByNumber(i);
                    if(tmpBl.isVisible()) {
                        tmpBl.setVisible(false);
                        tmpBl.setLineCount(0);
                    }
                }
            }
        }
    }
    EditorLayout *layout = static_cast<EditorLayout *>(document()->documentLayout());
    layout->requestUpdate();
    layout->emitDocumentSizeChanged();
}