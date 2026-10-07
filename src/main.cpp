#include "main_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Easy Vulkan QEMU"));
    application.setOrganizationName(QStringLiteral("easy-vulkan-qemu"));

    launcher::MainWindow window;
    window.show();
    return application.exec();
}
