#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("FloatingTranslator"));
    QApplication::setApplicationName(QStringLiteral("FloatingTranslator"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/logo.svg")));

    MainWindow window;
    window.show();
    return app.exec();
}
