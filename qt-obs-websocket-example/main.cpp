#include <QCoreApplication>
#include <QtTest/QTest>
#include <qtobswebsocket.h>

int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);

  {
    QtObsWebsocket obs_socket;
    auto version = obs_socket.getVersion();
    qInfo() << "Version info: " << version.OBSStudioVersion << " Plugin: " << version.PluginVersion;
    obs_socket.startRecording();
    QTest::qSleep(5000);
    obs_socket.stopRecording();
  }

  return a.exec();
}
