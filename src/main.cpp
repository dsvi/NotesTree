#include "NotesTreeModel.h"

int main(int argc, char *argv[])
{
  QGuiApplication app(argc, argv);

	qmlRegisterType<NotesTreeModel>("ds.OverNote",1,0,"NotesTreeModel");

  QQmlApplicationEngine engine;
  engine.load(QUrl(QStringLiteral("qrc:/main.qml")));


	Note dir;
	dir.createHierarchyFromRoot(QDir{"/home/ds/OTest"});

  return app.exec();
}

