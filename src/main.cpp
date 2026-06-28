#include <QApplication>
#include <QMainWindow>
#include <QLabel>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VEX V5 Path Planner");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Kadir Arslan Ünal");

    QMainWindow window;
    window.setWindowTitle("VEX V5 Path Planner");
    window.resize(1280, 800);

    auto* label = new QLabel("Phase 1 complete — core math layer loaded.", &window);
    label->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(label);

    window.show();
    return app.exec();
}
