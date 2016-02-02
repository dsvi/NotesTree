#include "NotesTreeModel.h"
#include "MainWindow.h"
#include "App.h"


void MessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
		QByteArray localMsg = msg.toLocal8Bit();
		switch (type) {
		case QtDebugMsg:
				//fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
				fprintf(stderr, "%s\n", localMsg.constData());
				break;
		case QtInfoMsg:
				fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
				break;
		case QtWarningMsg:
				fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
				break;
		case QtCriticalMsg:
				fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
				break;
		case QtFatalMsg:
				fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
				abort();
		}
}

int main(int argc, char *argv[])
{
  //setenv("QT_DEVICE_PIXEL_RATIO","auto",0);
#ifdef DEBUG
	qInstallMessageHandler(MessageOutput);
#endif
	QApplication a(argc, argv);
	a.setApplicationName("TreeNote");
	app = new App(&a);
	try{
		MainWindow w;
		w.show();
		app->mainWindow(&w);

		return a.exec();
	}
	catch(std::exception &e){
		emit app->showErrorDilogSlot(std::current_exception());
	}
}


