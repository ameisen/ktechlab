#include "pch.hpp"

#include "ktechlab.h"
#include "diagnosticstyle.h"
#include "logtofilemsghandler.h"

#include <KConfig>
#include <config.h>

#include <KAboutData>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>

#if defined(__SANITIZE_ADDRESS__)
#	include <sanitizer/lsan_interface.h>
#endif

namespace Application {
	static constexpr const cstring Name = "ktechlab";
	static constexpr const cstring Version = VERSION;
	static constexpr const cstring Description = "An IDE for microcontrollers and electronics";
	static constexpr const cstring OtherInfo = "";
	static constexpr const cstring DisplayName = "KTechLab";
	static constexpr const KAboutLicense::LicenseKey License = KAboutLicense::GPL_V2;
	static constexpr const cstring Copyright = "© 2003-2020, The KTechLab developers";
	static constexpr const cstring URI = "https://userbase.kde.org/KTechlab";
	static constexpr const cstring EMail = "ktechlab-devel@kde.org";
}

namespace {
	struct Credit final {
		const char * const name;
		const char * const task = nullptr;
		const char * const email = nullptr;
		const char * const uri = nullptr;
		const char * const ocs = nullptr;

		template <typename T>
		void add(KAboutData &about, const T &addFunctor) const {
			(about.*addFunctor)(
				localize(name),
				localize(task),
				email ? email : QString{},
				uri ? uri : QString{},
				ocs ? ocs : QString{}
			);
		}

		void addAuthor(KAboutData &about) const {
			add(about, &KAboutData::addAuthor);
		}

		void addCredit(KAboutData &about) const {
			add(about, &KAboutData::addCredit);
		}
	};

	static constexpr const Credit authors[] = {
		{
			"Alan Grimes",
			"Developer, Simulation"
		},
		{
			"Zoltan Padrah",
			"Developer",
			"zoltan_padrah@users.sourceforge.net",
			"https://github.com/zoltanp"
		},
		{
			"Julian Bäume",
			"Developer, KDE4 Port, GUI",
			"julian@svg4all.de"
		},
		{
			"Juan De Vincenzo",
			"KDE4 Port"
		},
		{
			"Michael Kuklinski",
			"C++ Modernization, Refactoring, Crash Resolution",
			nullptr,
			"https://github.com/ameisen"
		}
	};

	static constexpr const Credit credits[] = {
		{
			"Lawrence Shafer",
			"Website, wiki and forum"
		},
		{
			"Jason Lucas",
			"Keeping up the project during lack of developers"
		},
		{
			"David Saxton",
			"Former developer, project founder, former maintainer",
			"david@bluehaze.org"
		},
		{
			"Daniel Clarke",
			"Former developer",
			"daniel.jc@gmail.com"
		},
		{
			"'Couriousous'",
			"JK flip-flop, asynchronous preset/reset in the D flip-flop"
		},
		{
			"John Myers",
			"Rotary Switch"
		},
		{
			"Ali Akcaagac",
			"Glib friendliness",
		},
		{
			"David Leggett",
			"Former website hosting and feedback during early development",
		}
	};
}

int main(int argc, char **argv) {
	LogToFileMsgHandler logFileHandler;
	QApplication app{argc, argv};
	KLocalizedString::setApplicationDomain(Application::Name);

	KAboutData::setApplicationData([](){
		auto about = KAboutData{
			Application::Name,
			localize(Application::DisplayName),
			Application::Version,
			localize(Application::Description),
			Application::License,
			localize(Application::Copyright),
			localize(Application::OtherInfo),
			Application::URI,
			Application::EMail
		};

		for (const auto &author : authors) {
			author.addAuthor(about);
		}

		for (const auto &credit : credits) {
			credit.addCredit(about);
		}

		return about;
	}());

	auto * const ktechlab = new KTechlab;

	{
		// https://techbase.kde.org/Development/Tutorials/KCmdLineArgs
		QCommandLineParser parser;
		parser.addHelpOption();
		parser.addVersionOption();
		// 2019.10.03 - note: to add options to set icon and caption of the
		//              application's window? currently this is not implemented
		//              but it had references in the .destop file
		parser.addPositionalArgument(
			QStringLiteral("[URL]"),
			localize("Document to open.")
		);

		KAboutData::applicationData().setupCommandLine(&parser);
		parser.process(app);

		// 2019.10.03 - note: possibly add support for multiple URLs to be opened from
		//              command line?
		if (!parser.positionalArguments().isEmpty()) {
			ktechlab->load(parser.positionalArguments().first());
		}
	}

	ktechlab->show();

	const int resultCode = app.exec();

	// https://stackoverflow.com/a/51553776
#if defined(__SANITIZE_ADDRESS__)
	__lsan_do_leak_check();
	__lsan_disable();
#endif

	return resultCode;
}
