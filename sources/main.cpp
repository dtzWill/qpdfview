/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"

struct File
{
    QString filePath;
    int page;
};

int main(int argc, char** argv)
{
    QApplication application(argc, argv);

    QApplication::setOrganizationDomain("local.qpdfview");
    QApplication::setOrganizationName("qpdfview");
    QApplication::setApplicationName("qpdfview");

    QApplication::setApplicationVersion("0.3.1.99");

    QApplication::setWindowIcon(QIcon(":icons/qpdfview.svg"));

    QTranslator translator;

    if(translator.load(QString("%1/qpdfview_%2").arg(DATA_INSTALL_PATH).arg(QLocale::system().name())))
    {
            application.installTranslator(&translator);
    }

    bool unique = false;
    QList< File > files;

    {
        // command-line arguments

        QStringList arguments = QApplication::arguments();

        if(!arguments.isEmpty())
        {
            arguments.removeFirst();
        }

        foreach(QString argument, arguments)
        {
            if(argument == "--unique")
            {
                unique = true;
            }
            else
            {
                QStringList fields = argument.split('#');

                File file;

                file.filePath = QFileInfo(fields.value(0)).absoluteFilePath();
                file.page = fields.value(1).toInt();

                files.append(file);
            }
        }
    }

    MainWindow* mainWindow = 0;

#ifdef WITH_DBUS

    {
        // D-Bus

        if(unique)
        {
            QDBusInterface* interface = new QDBusInterface("local.qpdfview", "/MainWindow", "local.qpdfview.MainWindow", QDBusConnection::sessionBus());

            if(interface->isValid())
            {
                foreach(File file, files)
                {
                    QDBusReply< void > reply = interface->call("refreshOrOpenInNewTab", file.filePath, file.page);

                    if(!reply.isValid())
                    {
                        qDebug() << QDBusConnection::sessionBus().lastError().message();

                        delete interface;
                        return 1;
                    }
                }

                delete interface;
                return 0;
            }
            else
            {
                mainWindow = new MainWindow();

                new MainWindowAdaptor(mainWindow);

                if(!QDBusConnection::sessionBus().registerService("local.qpdfview"))
                {
                    qDebug() << QDBusConnection::sessionBus().lastError().message();

                    delete mainWindow;
                    return 1;
                }

                if(!QDBusConnection::sessionBus().registerObject("/MainWindow", mainWindow))
                {
                    qDebug() << QDBusConnection::sessionBus().lastError().message();

                    delete mainWindow;
                    return 1;
                }
            }
        }
        else
        {
            mainWindow = new MainWindow();
        }
    }

#else

    mainWindow = new MainWindow();

#endif // WITH_DBUS

    mainWindow->show();
    mainWindow->setAttribute(Qt::WA_DeleteOnClose);

    foreach(File file, files)
    {
        mainWindow->openInNewTab(file.filePath, file.page);
    }
    
    return application.exec();
}