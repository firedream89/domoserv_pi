#include "interface.h"

Interface::Interface()
{
    CVOrder *cvOrder = new CVOrder;
    Server *server = new Server;
    connect(server,SIGNAL(Receipt(QTcpSocket*,QString)),cvOrder,SLOT(ReceiptDataFromUser(QTcpSocket*,QString)));

    Test();
}

bool Interface::Test()
{
    return true;
}

void Interface::pong(quint64 msec)
{
    qDebug() << "Pong " << msec << "\n";
}
