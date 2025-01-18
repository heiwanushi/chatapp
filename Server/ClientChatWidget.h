#ifndef CLIENTCHATWIDGET_H
#define CLIENTCHATWIDGET_H

#include <QDir>
#include <QTcpSocket>
#include <QWidget>
#include <QObject>
#include "ChatProtocol.h"

namespace Ui {
class ClientChatWidget;
}

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QHostAddress ip = QHostAddress::LocalHost, ushort port = 4500, QObject *parent = nullptr);
    explicit ClientManager(QTcpSocket *client, QObject *parent = nullptr);

    void connectToServer();
    void disconnectFromHost();

    void sendMessage(QString message);
    void sendName(QString name);
    void sendStatus(ChatProtocol::Status status);
    QString name() const;
    void sendIsTyping();
    void sendInitSendingFile(QString fileName);
    void sendAcceptFile();
    void sendRejectFile();

public:
    QTcpSocket* socket() const;

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString message, QString receiver, QString sender);
    void isTyping();
    void nameChanged(QString prevName, QString name);
    void statusChanged(ChatProtocol::Status status);
    void rejectReceivingFile();
    void initReceivingFile(QString clientName, QString fileName, qint64 fileSize);
    void fileSaved(QString path);

private slots:
    void readyRead();

private:
    QTcpSocket *_socket;
    QHostAddress _ip;
    ushort _port;
    ChatProtocol _protocol;
    QString _tmpFileName;

    void setupClient();
    void sendFile();
    void saveFile();
};

class ClientChatWidget : public QWidget
{
    Q_OBJECT

public:
    QTcpSocket* client() const;

public:
    explicit ClientChatWidget(QTcpSocket *client, QWidget *parent = nullptr);
    void disconnect();
    ~ClientChatWidget();

private slots:
    void clientDisconnected();
    void on_btnSend_clicked();
    void textMessageReceived(QString message, QString receiver, QString sender);
    void onTyping();
    void onInitReceivingFile(QString clientName, QString fileName, qint64 fileSize);
    void onFileSaved(QString path);
    void on_lblOpenFolder_linkActivated(const QString &link);
    void onClientNameChanged(QString prevName, QString name);

signals:
    void clientNameChanged(QString prevName, QString name);
    void isTyping(QString message);
    void statusChanged(ChatProtocol::Status status);
    void textForOtherClients(QString message, QString receiver, QString sender);

private:
    Ui::ClientChatWidget *ui;
    ClientManager *_client;
    QDir dir;
};

#endif // CLIENTCHATWIDGET_H
