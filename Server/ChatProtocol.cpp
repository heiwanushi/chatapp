#include "ChatProtocol.h"

#include <QFileInfo>
#include <QIODevice>

ChatProtocol::ChatProtocol()
{

}

// Функция для создания текстового сообщения
QByteArray ChatProtocol::textMessage(QString message, QString receiver, QString sender)
{
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << Text << receiver << sender << message; // Запись типа сообщения, получателя, отправителя и самого сообщения
    return ba;
}

// Функция для создания сообщения "печатает"
QByteArray ChatProtocol::isTypingMessage()
{
    return getData(IsTyping, ""); // Вызов вспомогательной функции для создания сообщения
}

// Функция для создания сообщения об установке имени
QByteArray ChatProtocol::setNameMessage(QString name)
{
    return getData(SetName, name); // Вызов вспомогательной функции для создания сообщения
}

// Функция для создания сообщения об установке статуса
QByteArray ChatProtocol::setStatusMessage(Status status)
{
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << SetStatus << status; // Запись типа сообщения и статуса
    return ba;
}

// Функция для создания сообщения о начале отправки файла
QByteArray ChatProtocol::setInitSendingFileMessage(QString fileName)
{
    QByteArray ba;
    QFileInfo info(fileName);
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << InitSendingFile << info.fileName() << info.size(); // Запись типа сообщения, имени файла и его размера
    return ba;
}

// Функция для создания сообщения о принятии файла
QByteArray ChatProtocol::setAcceptFileMessage()
{
    return getData(AcceptSendingFile, ""); // Вызов вспомогательной функции для создания сообщения
}

// Функция для создания сообщения об отклонении файла
QByteArray ChatProtocol::setRejectFileMessage()
{
    return getData(RejectSendingFile, ""); // Вызов вспомогательной функции для создания сообщения
}

// Функция для создания сообщения с файлом
QByteArray ChatProtocol::setFileMessage(QString fileName)
{
    QByteArray ba;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QFileInfo info(fileName);
        QDataStream out(&ba, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_6_0);
        out << SendFile << info.fileName() << info.size() << file.readAll(); // Запись типа сообщения, имени файла, его размера и содержимого
        file.close();
    }
    return ba;
}

// Функция для создания сообщения об изменении имени клиента
QByteArray ChatProtocol::setClientNameMessage(QString prevName, QString name)
{
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << ClientName << prevName << name; // Запись типа сообщения, предыдущего и нового имени клиента
    return ba;
}

// Функция для создания сообщения о подтверждении соединения
QByteArray ChatProtocol::setConnectionACKMessage(QString clientName, QStringList otherClients)
{
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << ConnectionACK << clientName << otherClients; // Запись типа сообщения, имени клиента и списка других клиентов
    return ba;
}

// Функция для создания сообщения о новом клиенте
QByteArray ChatProtocol::setNewClientMessage(QString clientName)
{
    return getData(NewClient, clientName); // Вызов вспомогательной функции для создания сообщения
}

// Функция для создания сообщения об отключении клиента
QByteArray ChatProtocol::setClientDisconnectedMessage(QString clientName)
{
    return getData(ClientDisconnected, clientName); // Вызов вспомогательной функции для создания сообщения
}

// Функция для загрузки данных из сообщения
void ChatProtocol::loadData(QByteArray data)
{
    QDataStream in(&data, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_6_0);
    in >> _type;
    switch (_type) {
    case Text:
        in >> _receiver >> _sender >> _message; // Чтение получателя, отправителя и сообщения
        break;
    case SetName:
        in >> _name; // Чтение имени
        break;
    case SetStatus:
        in >> _status; // Чтение статуса
        break;
    case InitSendingFile:
        in >> _fileName >> _fileSize; // Чтение имени файла и его размера
        break;
    case SendFile:
        in >> _fileName >> _fileSize >> _fileData; // Чтение имени файла, его размера и содержимого
        break;
    default:
        break;
    }
}

// Вспомогательная функция для создания сообщения с данными
QByteArray ChatProtocol::getData(MessageType type, QString data)
{
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << type << data; // Запись типа сообщения и данных
    return ba;
}

// Геттер для получателя
const QString &ChatProtocol::receiver() const
{
    return _receiver;
}

// Геттер для отправителя
const QString &ChatProtocol::sender() const
{
    return _sender;
}

// Геттер для данных файла
const QByteArray &ChatProtocol::fileData() const
{
    return _fileData;
}

// Геттер для размера файла
qint64 ChatProtocol::fileSize() const
{
    return _fileSize;
}

// Геттер для имени файла
const QString &ChatProtocol::fileName() const
{
    return _fileName;
}

// Геттер для типа сообщения
ChatProtocol::MessageType ChatProtocol::type() const
{
    return _type;
}

// Геттер для статуса
ChatProtocol::Status ChatProtocol::status() const
{
    return _status;
}

// Геттер для имени
const QString &ChatProtocol::name() const
{
    return _name;
}

// Геттер для сообщения
const QString &ChatProtocol::message() const
{
    return _message;
}
