#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QtNetwork>
#include <QWebSocket>
#include <QWebSocketServer>

class Server : public QObject
{
    Q_OBJECT
public:
    Server();
    bool StartServer();
    void SendToAll(QString data);
    void SendToUser(QTcpSocket *user, QString data);

private slots:
    void NewConnexion();
    void ReceiptData();
    void Disconnect();
    void AddUserToList(QTcpSocket *socket, QString data);

signals:
    void Receipt(QTcpSocket *client, QString data);

private:
    QString Encrypt(QString text);
    QString Decrypt(QString text);
    void GeneratePKEY();

    QTcpServer *server;
    QList<QTcpSocket*> usersList;
    int dataSize;
    QString password;
    QString PKEY;
};

#endif // SERVER_H
