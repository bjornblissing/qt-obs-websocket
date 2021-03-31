#ifndef QTOBSWEBSOCKET_H
#define QTOBSWEBSOCKET_H

#include <QString>
#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>

#include <obsresponse.h>
#include <optional>

class QtObsWebsocket : public QObject {
  Q_OBJECT
 public:
  QtObsWebsocket();
  QtObsWebsocket(std::optional<QString> password);
  QtObsWebsocket(QUrl url, std::optional<QString> password);
  QtObsWebsocket(QString ip_address, quint16 port, std::optional<QString> password);
  ~QtObsWebsocket();
  OBSVersion getVersion();
  OBSAuthInfo getAuthInfo();
  void startRecording();
  void stopRecording();
  void startStreaming();
  void stopStreaming();
  void setCurrentProfile(QString profile);
  void setCurrentScene(QString scene);

 private Q_SLOTS:
  void onConnected();
  void onDisconnected();
  void onTextMessageReceived(QString message);

 private:
  void connect(std::optional<QString> password);
  void disconnect();
  [[nodiscard]] bool authenticate(QString password, OBSAuthInfo authInfo);

  void sendAuthenticatedCommand(QString command,
                                std::optional<QJsonObject> parameters = std::nullopt);

  [[nodiscard]] QJsonObject sendRequest(
    QString requestType,
    std::optional<QJsonObject> requestParameters = std::nullopt);

  [[nodiscard]] QString generateMessageId(const size_t length = 16);
  [[nodiscard]] QString hashEncode(QString input);
  [[nodiscard]] QJsonObject responseToJsonObject(QString reponse);
  [[nodiscard]] QString jsonObjectToString(QJsonObject request);

  QWebSocket m_socket;
  QString m_response = {};
  QUrl m_url = QUrl("ws://127.0.0.1:4444");
  bool m_authenticated = {false};
};

#endif  // QTOBSWEBSOCKET_H
