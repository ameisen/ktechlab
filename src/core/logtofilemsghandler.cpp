#include "logtofilemsghandler.h"

#include <QtGlobal>
#include <QDebug>
#include <QDate>
#include <QApplication>

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <errno.h>

namespace {
	static FILE *logFile = nullptr;

	static int closeLogFile() {
		if (!logFile) return 0;
		const int result = fclose(logFile);
		logFile = nullptr;
		return result;
	}
}

static void ktlLogToFile(QtMsgType type, const char *msg) {
	auto nowStr = QDateTime::currentDateTime().toString("yy-MM-dd hh:mm:ss,zzz");
	const QByteArray latin1Text = nowStr.toLatin1();
	const char *nowCStr = latin1Text.data();

	FILE *outStream = nullptr;
	const char *formatString = nullptr;
	switch (type) {
	case QtDebugMsg:
		outStream = logFile;
		formatString = "%s (DD) %s\n";
		break;
	case QtWarningMsg:
		outStream = logFile;
		formatString = "%s (WW) %s\n";
		break;
	case QtCriticalMsg:
		outStream = logFile;
		formatString = "%s (Critical) %s\n";
		break;
	case QtFatalMsg:
		outStream = stderr;
		formatString = " %s (Fatal) %s\n";
		break;
	default:
		return;
	}

	if (outStream && formatString) {
		fprintf(outStream, formatString, nowCStr, msg);
		fflush(outStream);
	}
}

static void ktlLogToStderr(QtMsgType type, const char *msg) {
	switch (type) {
	case QtWarningMsg:
		fprintf(stderr, "(WW) %s\n", msg);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "(Critical) %s\n", msg);
		break;
	case QtFatalMsg:
		fprintf(stderr, "(Fatal) %s\n", msg);
		break;
	default:
		return;
	}
	fflush(stderr);
}

static void ktlMessageOutput(QtMsgType type, const char *msg) {
	ktlLogToFile(type, msg);
	ktlLogToStderr(type, msg);
	if (QtFatalMsg == type) {
		abort();
	}
}

LogToFileMsgHandler::LogToFileMsgHandler() {
	closeLogFile();

	auto appPid = QApplication::applicationPid();
	auto logFileName = QString("/tmp/ktechlab-pid-%1-log").arg(appPid);

	qDebug() << "Starting logging to " << logFileName;

	logFile = fopen(logFileName.toLatin1().data(), "w+");

	if (!logFile) {
		const auto lastErrno = errno;
		qWarning()
			<< "Failed to create log file" << logFileName
			<< ". errno=" << lastErrno << ", strerror=" << strerror(lastErrno);
		return;
	}
	qInstallMsgHandler(ktlMessageOutput);
	qDebug() << "logging started to " << logFileName << " by " << this;
}

LogToFileMsgHandler::~LogToFileMsgHandler() {
	qDebug() << "logging ending by " << this;
	const int closeRet = closeLogFile();
	if (closeRet) {
		const auto lastErrno = errno;
		qCritical() << "failed to close log file, errno=" << lastErrno;
	}
	qInstallMsgHandler(0);
	if (closeRet) {
		const auto lastErrno = errno;
		qCritical() << "failed to close log file, errno=" << lastErrno;
	}
}
