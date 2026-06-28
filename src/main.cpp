#include <QApplication>
#include <QPalette>
#include <QColor>
#include "ui/MainWindow.h"

static void applyDarkTheme(QApplication& app) {
    app.setStyle("Fusion");
    QPalette p;
    p.setColor(QPalette::Window,          QColor(30, 31, 34));
    p.setColor(QPalette::WindowText,      QColor(210, 210, 215));
    p.setColor(QPalette::Base,            QColor(20, 21, 24));
    p.setColor(QPalette::AlternateBase,   QColor(38, 39, 43));
    p.setColor(QPalette::ToolTipBase,     QColor(25, 26, 30));
    p.setColor(QPalette::ToolTipText,     QColor(210, 210, 215));
    p.setColor(QPalette::Text,            QColor(210, 210, 215));
    p.setColor(QPalette::Button,          QColor(45, 47, 52));
    p.setColor(QPalette::ButtonText,      QColor(210, 210, 215));
    p.setColor(QPalette::BrightText,      Qt::red);
    p.setColor(QPalette::Highlight,       QColor(66, 133, 244));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(85, 86, 90));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(85, 86, 90));
    app.setPalette(p);
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VEX V5 Path Planner");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Kadir Arslan Ünal");

    applyDarkTheme(app);

    MainWindow window;
    window.show();
    return app.exec();
}
