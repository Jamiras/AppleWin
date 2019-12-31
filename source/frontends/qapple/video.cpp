#include "video.h"

#include <QPainter>
#include <QKeyEvent>

#include "StdAfx.h"
#include "linux/data.h"
#include "linux/keyboard.h"
#include "Common.h"
#include "CardManager.h"
#include "MouseInterface.h"
#include "Applewin.h"

Video::Video(QWidget *parent) : VIDEO_BASECLASS(parent)
{
    setMouseTracking(true);

    myLogo = QImage(":/resources/APPLEWINLOGO.BMP").mirrored(false, true);
}

QImage Video::getScreen() const
{
    uint8_t * data;
    int width;
    int height;
    int sx, sy;
    int sw, sh;

    getScreenData(data, width, height, sx, sy, sw, sh);
    QImage frameBuffer(data, width, height, QImage::Format_ARGB32_Premultiplied);

    QImage screen = frameBuffer.copy(sx, sy, sw, sh);

    return screen;
}

void Video::displayLogo()
{
    uint8_t * data;
    int width;
    int height;
    int sx, sy;
    int sw, sh;

    getScreenData(data, width, height, sx, sy, sw, sh);
    QImage frameBuffer(data, width, height, QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&frameBuffer);
    painter.drawImage(sx, sy, myLogo);
}

void Video::paintEvent(QPaintEvent *)
{
    uint8_t * data;
    int width;
    int height;
    int sx, sy;
    int sw, sh;

    getScreenData(data, width, height, sx, sy, sw, sh);
    QImage frameBuffer(data, width, height, QImage::Format_ARGB32_Premultiplied);

    const QSize actual = size();
    const double scaleX = double(actual.width()) / sw;
    const double scaleY = double(actual.height()) / sh;

    // then paint it on the widget with scale
    {
        QPainter painter(this);

        // scale and flip vertically
        const QTransform transform(scaleX, 0.0, 0.0, -scaleY, 0.0, actual.height());
        painter.setTransform(transform);

        painter.drawImage(0, 0, frameBuffer, sx, sy, sw, sh);
    }
}

void Video::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    BYTE ch = 0;

    switch (key)
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        ch = 0x0d;
        break;
    case Qt::Key_Left:
        ch = 0x08;
        break;
    case Qt::Key_Right:
        ch = 0x15;
        break;
    case Qt::Key_Up:
        ch = 0x0b;
        break;
    case Qt::Key_Down:
        ch = 0x0a;
        break;
    case Qt::Key_Delete:
        ch = 0x7f;
        break;
    case Qt::Key_Escape:
        ch = 0x1b;
        break;
    default:
        if (key < 0x80)
        {
            ch = key;
            if (ch >= 'A' && ch <= 'Z')
            {
                const Qt::KeyboardModifiers modifiers = event->modifiers();
                if (modifiers & Qt::ShiftModifier)
                {
                    ch += 'a' - 'A';
                }
            }
        }
        break;
    }

    if (ch)
    {
        addKeyToBuffer(ch);
        event->accept();
    }
}

void Video::mouseMoveEvent(QMouseEvent *event)
{
    if (g_CardMgr.IsMouseCardInstalled() && g_CardMgr.GetMouseCard()->IsActiveAndEnabled())
    {
        int iX, iMinX, iMaxX;
        int iY, iMinY, iMaxY;
        g_CardMgr.GetMouseCard()->GetXY(iX, iMinX, iMaxX, iY, iMinY, iMaxY);

        const QPointF p = event->localPos();
        const QSize s = size();

        const int newX = lround((p.x() / s.width()) * (iMaxX - iMinX) + iMinX);
        const int newY = lround((p.y() / s.height()) * (iMaxY - iMinY) + iMinY);

        const int dx = newX - iX;
        const int dy = newY - iY;

        int outOfBoundsX;
        int outOfBoundsY;
        g_CardMgr.GetMouseCard()->SetPositionRel(dx, dy, &outOfBoundsX, &outOfBoundsY);

        event->accept();
    }
}

void Video::mousePressEvent(QMouseEvent *event)
{
    if (g_CardMgr.IsMouseCardInstalled() && g_CardMgr.GetMouseCard()->IsActiveAndEnabled())
    {
        Qt::MouseButton button = event->button();
        switch (button)
        {
        case Qt::LeftButton:
            g_CardMgr.GetMouseCard()->SetButton(BUTTON0, BUTTON_DOWN);
            break;
        case Qt::RightButton:
            g_CardMgr.GetMouseCard()->SetButton(BUTTON1, BUTTON_DOWN);
            break;
        default:
            break;
        }
        event->accept();
    }
}

void Video::mouseReleaseEvent(QMouseEvent *event)
{
    if (g_CardMgr.IsMouseCardInstalled() && g_CardMgr.GetMouseCard()->IsActiveAndEnabled())
    {
        Qt::MouseButton button = event->button();
        switch (button)
        {
        case Qt::LeftButton:
            g_CardMgr.GetMouseCard()->SetButton(BUTTON0, BUTTON_UP);
            break;
        case Qt::RightButton:
            g_CardMgr.GetMouseCard()->SetButton(BUTTON1, BUTTON_UP);
            break;
        default:
            break;
        }
        event->accept();
    }
}
