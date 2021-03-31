#include <QCoreApplication>
#include <qtobswebsocket.h>

int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);

  {
    QtObsWebsocket obs_socket;
    auto version = obs_socket.getVersion();
    qInfo() << "Version info: " << version.OBSStudioVersion << " Plugin: " << version.PluginVersion;
  }

  return a.exec();
}
