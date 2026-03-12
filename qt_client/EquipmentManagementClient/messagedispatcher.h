#ifndef MESSAGEDISPATCHER_H
#define MESSAGEDISPATCHER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <functional>
#include "protocol_parser.h"

class TcpClient;

class MessageDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit MessageDispatcher(TcpClient *tcpClient, QObject *parent = nullptr);
    ~MessageDispatcher();

    void registerHandler(ProtocolParser::MessageType type, std::function<void(const ProtocolParser::ParseResult&)> handler);
    void unregisterHandler(ProtocolParser::MessageType type);

signals:
    void heartbeatResponseReceived(const QString &fromEquipmentId);

public slots:
    void onProtocolMessageReceived(const ProtocolParser::ParseResult &result);

private:
    TcpClient *m_tcpClient;
    QMap<ProtocolParser::MessageType, QList<std::function<void(const ProtocolParser::ParseResult&)>>> m_handlerMap;
};

#endif // MESSAGEDISPATCHER_H
