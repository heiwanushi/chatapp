#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QTcpSocket>
#include "ChatProtocol.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ClientManager : public QObject
{
    Q_OBJECT
public:
    explicit ClientManager(QHostAddress ip = QHostAddress::LocalHost, ushort port = 4500, QObject *parent = nullptr);

    void connectToServer();

    void sendMessage(QString message, QString receiver);
    void sendName(QString name);
    void sendStatus(ChatProtocol::Status status);
    void sendIsTyping();
    void setClientName(const QString &name); // Добавлено

    void sendInitSendingFile(QString fileName);
    void sendAcceptFile();
    void sendRejectFile();

signals:
    void connected();
    void disconnected();
    void textMessageReceived(QString message, QString sender);
    void isTyping();
    void nameChanged(QString name);
    void statusChanged(ChatProtocol::Status status);
    void rejectReceivingFile();
    void initReceivingFile(QString clientName, QString fileName, qint64 fileSize);

    void connectionACK(QString myName, QStringList clientsName);
    void newClientConnectedToServer(QString clientName);
    void clientNameChanged(QString prevName, QString clientName);
    void clientDisconnected(QString clientName);

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
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionConnect_triggered();
    void on_btnSend_clicked();
    void dataReceived(QString message, QString sender);
    void on_lnClientName_editingFinished();
    void on_cmbStatus_currentIndexChanged(int index);
    void onTyping();
    void on_btnSendFile_clicked();
    void onRejectReceivingFile();
    void onInitReceivingFile(QString clientName, QString fileName, qint64 fileSize);
    void onConnectionACK(QString myName, QStringList clientsName);
    void onNewClientConnectedToServer(QString clientName);
    void onClientNameChanged(QString prevName, QString clientName);
    void onClientDisconnected(QString clientName);

private:
    Ui::MainWindow *ui;
    ClientManager *_client;
    void setupClient();
};

#endif // MAINWINDOW_H
