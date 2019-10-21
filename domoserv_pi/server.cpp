#include "server.h"

//Version 1.01

Server::Server()
{
    #define className "Serveur"

    server = new QTcpServer;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    UserServer = new QTcpServer;
    connect(UserServer,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
#ifdef WEBSECURED
    qDebug()<<"SSL version use for build: "<<QSslSocket::sslLibraryBuildVersionString();
    qDebug()<<"SSL version use for run-time: "<<QSslSocket::sslLibraryVersionNumber();
    webServer = new QWebSocketServer("webServer",QWebSocketServer::SecureMode);
    webAdminServer = new QWebSocketServer("webAdminServer",QWebSocketServer::SecureMode);
#else
    webServer = new QWebSocketServer("webServer",QWebSocketServer::NonSecureMode);
    webAdminServer = new QWebSocketServer("webAdminServer",QWebSocketServer::NonSecureMode);
#endif
    connect(webServer,SIGNAL(newConnection()),this,SLOT(NewWebConnexion()));
    connect(webAdminServer,SIGNAL(newConnection()),this,SLOT(NewWebConnexion()));

    QSqlQuery req;
    req.exec("SELECT * FROM General WHERE Name='Port'");
    if(!req.next())
    {
        req.exec("SELECT MAX(ID) FROM General");
        req.next();
        int id = req.value(0).toInt()+1;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Port','49152','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','ActAdminServer','0','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','Password','','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebSocket','0','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebAdminSocket','0','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPort','49155','','','')");
        id++;
        req.exec("INSERT INTO General VALUES('" + QString::number(id) + "','WebPassword','','','','')");
    }
}
/*
Server::~Server()
{
    Stop();

    server->deleteLater();
    UserServer->deleteLater();
    webAdminServer->deleteLater();
    webServer->deleteLater();
}*/

void Server::Stop()
{
    //TCP SERVER
    for(int i=0;i<adminList.count();i++)
    {
        emit Info(className,"Forced disconnect admin");
        adminList.at(i)->disconnectFromHost();
    }
    adminList.clear();
    //WEB SERVER
    for(int i=0;i<webAdminList.count();i++)
    {
        emit Info(className,"Forced disconnect web admin");
        webAdminList.at(i)->close();
    }
    webAdminList.clear();
    //TCP SERVER USER
    for(int i=0;i<usersList.count();i++)
    {
        emit Info(className,"Forced disconnect admin");
        usersList.at(i)->disconnectFromHost();
    }
    usersList.clear();
    //WEB SERVER USER
    for (int i=0;i<webUsersList.count();i++) {
        emit Info(className,"Forced disconnect web user");
        webUsersList.at(i)->close();
    }
    webUsersList.clear();
    //VARIABLES
    dataSize = 0;
    password.clear();
    webPassword.clear();

    emit Info(className,"Closing server...");
    server->close();
    webAdminServer->close();
    UserServer->close();
    webServer->close();

    //TEST
    bool test = true;
    if(server->isListening())
    {
        emit Info(className,"Admin Server not closed !");
        test = false;
    }
    if(webAdminServer->isListening())
    {
        emit Info(className,"Web Admin Server not closed !");
        test = false;
    }
    if(UserServer->isListening())
    {
        emit Info(className,"User Server not closed !");
        test = false;
    }
    if(webServer->isListening())
    {
        emit Info(className,"Web User Server not closed !");
        test = false;
    }
    if(test)
        emit Info(className,"[\033[0;32m  OK  \033[0m] Server closed");
    else
        emit Info(className,"[\033[0;31mFAILED\033[0m] Server not closed");
}

void Server::Reload()
{
    Stop();

    //INIT SERVER
    server = new QTcpServer;
    connect(server,SIGNAL(newConnection()),this,SLOT(NewConnexion()));
    UserServer = new QTcpServer;
    connect(UserServer,SIGNAL(newConnection()),this,SLOT(NewConnexion()));

#ifdef WEBSECURED
    webServer = new QWebSocketServer("webServer",QWebSocketServer::SecureMode);
    webAdminServer = new QWebSocketServer("webAdminServer",QWebSocketServer::SecureMode);
#else
    webServer = new QWebSocketServer("webServer",QWebSocketServer::NonSecureMode);
    webAdminServer = new QWebSocketServer("webAdminServer",QWebSocketServer::NonSecureMode);
#endif

    Init();
}

void Server::Init()
{
    emit Info(className,"Starting server");
    dataSize = 0;

    //Server Admin
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='ActAdminServer'");
    req.next();
    if(req.value(0).toInt() == 1) {
        //Password
        emit Info(className,"------------------Server Admin Info-------------------");
        req.exec("SELECT Value1 FROM General WHERE Name='Password'");
        req.next();
        password = req.value(0).toString();
        emit Info(className,"Password = " + password + "");

        //Port
        req.exec("SELECT Value1 FROM General WHERE Name='Port'");
        req.next();
        emit Info(className,"Port = " + req.value(0).toString());

        //Run server
        if(StartServer())
            emit Info(className,"[\033[0;32m  OK  \033[0m] Server started");
        else
            emit Info(className,"[\033[0;31mFAILED\033[0m] starting server failed");

        emit Info(className,"------------------------------------------------------");
    }


    //Server User
    emit Info(className,"------------------Server User Info--------------------");

    //webPassword
    req.exec("SELECT Value1 FROM General WHERE Name='WebPassword'");
    req.next();
    webPassword = req.value(0).toString();
    emit Info(className,"Password = " + webPassword + "");

    //Port
    req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
    req.next();
    emit Info(className,"Port = " + req.value(0).toString() + "");

    if(StartWebServer())
        emit Info(className,"[\033[0;32m  OK  \033[0m] Server started");
    else
        emit Info(className,"[\033[0;31mFAILED\033[0m] starting server failed");

    emit Info(className,"------------------------------------------------------");

    //CryptoFire
    crypto = new CryptoFire;
    crypto->Add_Encrypted_Key("Admin",password);
    crypto->Add_Encrypted_Key("User",webPassword);
}

bool Server::StartServer()
{    
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='Port'");
    req.next();
    int port = req.value(0).toInt();

    req.exec("SELECT Value1 FROM General WHERE Name='WebAdminSocket'");
    req.next();
    if(req.value(0).toInt() == 0)
    {
        emit Info(className,"Server type : TCP");
        return server->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
    else
    {
        emit Info(className,"Server type : Web Socket");
        return webAdminServer->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
}

void Server::NewConnexion()
{
    while(server->hasPendingConnections())
    {
        QTcpSocket *newCo = server->nextPendingConnection();

        connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
        connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
        emit Info(className,"New Admin connected(" + newCo->peerAddress().toString().toLatin1() + ")");

        SendToUser(newCo,crypto->Get_Key());
    }
    while(UserServer->hasPendingConnections())
    {
        QTcpSocket *newCo = UserServer->nextPendingConnection();

        connect(newCo,SIGNAL(readyRead()),this,SLOT(ReceiptData()));
        connect(newCo,SIGNAL(disconnected()),this,SLOT(Disconnect()));
        emit Info(className,"New User connected(" + newCo->peerAddress().toString().toLatin1() + ")");

        SendToUser(newCo,crypto->Get_Key());
    }
}

void Server::Disconnect()
{
    QTcpSocket *co = qobject_cast<QTcpSocket *>(sender());
    if(adminList.removeOne(co))
        emit Info(className,"Admin disconnected(" + co->peerAddress().toString().toLatin1() + ")");
    else if(usersList.removeOne(co))
        emit Info(className,"User disconnected(" + co->peerAddress().toString().toLatin1() + ")");
}

void Server::SendToUser(QTcpSocket *user, QString data)
{
    QByteArray paquet;

    QDataStream out(&paquet, QIODevice::WriteOnly);

    quint16 empty(0);
    out << empty;

    if(data.isEmpty())
        out << crypto->Get_Key();
    else {
        if(usersList.contains(user)) {
            crypto->Encrypt_Data(data,"User");
            out << data;
        }
        else {
            crypto->Encrypt_Data(data,"Admin");
            out << data;
        }
    }

    out.device()->seek(0);
    out << static_cast<quint16>(static_cast<uint>(paquet.size()) - sizeof(quint16));

    user->write(paquet);
}

void Server::ReceiptData()
{
    QTcpSocket *socket = new QTcpSocket;
    while(socket)
    {
        socket = qobject_cast<QTcpSocket *>(sender());

        if (!socket)
            return;

        QDataStream in(socket);

        if(dataSize == 0)
        {
            if(socket->bytesAvailable() < static_cast<uint>(sizeof(quint16)))
                 return;
            in >> dataSize;
        }

        if(socket->bytesAvailable() < dataSize)
            return;

        QString data;
        in >> data;

        if(!adminList.contains(socket) && !usersList.contains(socket))
        {
            AddUserToList(socket,data);
        }
        else
        {
            int privilege = User;
            if(adminList.contains(socket))
                privilege = Admin;
            if(privilege == User) {
                crypto->Decrypt_Data(data,"User");
                emit Receipt(socket,data,privilege);
            }
            else {
                crypto->Decrypt_Data(data,"Admin");
                emit Receipt(socket,data,privilege);
            }
        }

        dataSize = 0;
    }
}

void Server::AddUserToList(QTcpSocket *socket, QString data)
{
    //ADMIN
    if(socket->parent() == server)
    {
        crypto->Decrypt_Data(data,"Admin");
        if(data.contains("OK"))
        {
            if(data.split(" ").count() == 2) {
                socket->setObjectName(data.split(" ").last());
                adminList.append(socket);
                emit Info(className,"New admin accepted");
            }
            else {
                socket->close();
                emit Info(className,"New admin refused");
            }

        }
        else
        {
            socket->close();
            emit Info(className,"New admin refused");
        }
    }

    //USER
    if(socket->parent() == UserServer)
    {
        crypto->Decrypt_Data(data,"User");
        if(data.contains("OK"))
        {
            if(data.split(" ").count() == 2) {
                socket->setObjectName(data.split(" ").last());
                usersList.append(socket);
                emit Info(className,"New user accepted");
            }
            else {
                socket->close();
                emit Info(className,"New user refused");
            }
        }
        else
        {
            socket->close();
            emit Info(className,"New user refused");
        }
    }
}

bool Server::StartWebServer()
{
#ifdef WEBSECURED
        SecureWebSocket();
#endif
    QSqlQuery req;
    req.exec("SELECT Value1 FROM General WHERE Name='WebPort'");
    req.next();
    int port = req.value(0).toInt();

    req.exec("SELECT Value1 FROM General WHERE Name='WebSocket'");
    req.next();
    if(req.value(0).toInt() == 1)
    {
        emit Info(className,"Server type : Web Socket");
        return webServer->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
    else
    {
        emit Info(className,"Server type : TCP");
        return UserServer->listen(QHostAddress::Any,static_cast<quint16>(port));
    }
}

void Server::SecureWebSocket()
{
#ifdef WEBSECURED
    QSslConfiguration sslConfiguration;
    QFile certFile(QStringLiteral("CA/cacert.pem"));
    QFile keyFile(QStringLiteral("CA/private/cakey.pem"));
    certFile.open(QIODevice::ReadOnly);
    keyFile.open(QIODevice::ReadOnly);
    QSslCertificate certificate(&certFile, QSsl::Pem);
    QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
    certFile.close();
    keyFile.close();
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfiguration.setLocalCertificate(certificate);
    sslConfiguration.setPrivateKey(sslKey);
    sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
    webServer->setSslConfiguration(sslConfiguration);

    connect(webServer, &QWebSocketServer::sslErrors, this, &Server::SslErrors);
#endif

}

void Server::SslErrors(const QList<QSslError> &err)
{
#ifdef WEBSECURED
    for (int i = 0;i<err.count();i++) {
        emit Info(className,"SSL Error : " + err.at(i).errorString());
    }
#endif
}

void Server::NewWebConnexion()
{
    while(webServer->hasPendingConnections())
    {
        QWebSocket *socket = webServer->nextPendingConnection();
        socket->setParent(webServer);
        connect(socket,&QWebSocket::textMessageReceived,this,&Server::ReceiptMessage);
        connect(socket,&QWebSocket::disconnected,this,&Server::WebDisconnect);
        emit Info(className,"New Web user connected(" + socket->peerAddress().toString() + ")");

        SendToWebUser(socket,crypto->Get_Key());
    }
    while(webAdminServer->hasPendingConnections())
    {
        QWebSocket *socket = webAdminServer->nextPendingConnection();
        socket->setParent(webAdminServer);
        connect(socket,&QWebSocket::textMessageReceived,this,&Server::ReceiptMessage);
        connect(socket,&QWebSocket::disconnected,this,&Server::WebDisconnect);

        emit Info(className,"New Web Admin connected(" + socket->peerAddress().toString() + ")");

        SendToWebUser(socket,crypto->Get_Key());
    }
}

void Server::WebDisconnect()
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    emit Info(className,"Web user disconnected(" + socket->peerAddress().toString() + ")");
    webUsersList.removeOne(socket);
    webAdminList.removeOne(socket);
    socket->deleteLater();
}

void Server::ReceiptMessage(QString text)
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    if(socket)
    {
        //ADMIN
        if(socket->parent() == webAdminServer)
        {
            crypto->Decrypt_Data(text,"Admin");
            if(webAdminList.contains(socket))
            {
                #ifdef WEBSECURED
                    emit WebReceipt(socket,text,Admin);
                #else
                    emit WebReceipt(socket,text,Admin);
                #endif
            }
            else if(text.contains("OK"))
            {
                if(text.split(" ").count() == 2) {
                    socket->setObjectName(text.split(" ").last());
                    webAdminList.append(socket);
                    emit Info(className,"New web admin accepted");
                }
                else {
                    socket->close();
                    emit Info(className,"New web admin refused");
                }

            }
            else
            {
                socket->close();
                emit Info(className,"New web admin refused");
            }
        }

        //USER
        if(socket->parent() == webServer)
        {
            crypto->Decrypt_Data(text,"User");
            if(webUsersList.contains(socket))
            {
                #ifdef WEBSECURED
                    emit WebReceipt(socket,text, User);
                #else
                    emit WebReceipt(socket,text,User);
                #endif
            }
            else if(text.contains("OK"))
            {
                if(text.split(" ").count() == 2) {
                    socket->setObjectName(text.split(" ").last());
                    webUsersList.append(socket);
                    emit Info(className,"New web user accepted");
                }
                else {
                    socket->close();
                    emit Info(className,"New web user refused");
                }

            }
            else
            {
                socket->close();
                emit Info(className,"New web user refused");
            }
        }
    }

}

void Server::SendToWebUser(QWebSocket *socket, QString data)
{
    if(socket)
    {
        #ifdef WEBSECURED
            socket->sendTextMessage(data);
        #else
        if(webUsersList.contains(socket)) {
            crypto->Encrypt_Data(data,"User");
            socket->sendTextMessage(data);
        }
        else {
            socket->sendTextMessage(data);
        }
        #endif
    }
}
