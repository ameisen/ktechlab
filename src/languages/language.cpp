#include "asmparser.h"
#include "ktechlab.h"
#include "logview.h"
#include "language.h"
#include "outputmethoddlg.h"
#include "processchain.h"
#include "projectmanager.h"
#include "languagemanager.h"

#include <kmessagebox.h>
#include <kprocess.h>

#include <QDebug>
#include <QRegExp>
#include <QTimer>

#include <ktlconfig.h>

//BEGIN class Language
Language::Language(
	ProcessChain *processChain,
	const QString &name,
	const QString &successMessage,
	const QString &failureMessage
) :
	QObject(KTechlab::self()),
	successMessage_(i18n(successMessage)),
	failureMessage_(i18n(failureMessage)),
	processChain_(processChain)
{
	setObjectName(name);
}

void Language::outputMessage(const QString &message) {
	LanguageManager::self()->slotMessage(
		message,
		extractMessageInfo(message)
	);
}

void Language::outputWarning(const QString &message) {
	LanguageManager::self()->slotWarning(
		message, extractMessageInfo(message)
	);
}

void Language::outputError(const QString &message) {
	LanguageManager::self()->slotError(
		message,
		extractMessageInfo(message)
	);
	++errorCount_;
}

void Language::finish(Result result) {
	if (result == Result::Failure) {
		outputError(failureMessage_ + "\n");
		KTechlab::self()->slotChangeStatusbar(failureMessage_);
		emit processFailed(this);
		return;
	}

	outputMessage(successMessage_ + "\n");
	KTechlab::self()->slotChangeStatusbar(successMessage_);

	auto newPath = outputPath(processOptions_.processPath());

	if (newPath == ProcessOptions::Path::None) {
		emit processSucceeded(this);
	}
	else if (processChain_) {
		processOptions_.setInputFiles({ processOptions_.intermediaryOutput() });
		processOptions_.setIntermediaryOutput(processOptions_.targetFile());
		processOptions_.setProcessPath(newPath);
		processChain_->setProcessOptions(processOptions_);
		processChain_->compile();
	}
}

void Language::reset() {
	errorCount_ = 0;
}

MessageInfo Language::extractMessageInfo(const QString &text) {
	if (!text.startsWith("/")) {
		return MessageInfo{};
	}

	const int index = text.indexOf(
		":",
		0,
		Qt::CaseInsensitive
	);
	if (index == -1) {
		return MessageInfo{};
	}

	const auto fileName = text.left(index);

	// Extra line number
	const auto message = text.right(text.length() - index);
	const int linePos = message.indexOf(QRegExp(":[\\d]+"));
	int line = -1;
	if (linePos != -1) {
		const int linePosEnd = message.indexOf(':', linePos + 1);
		if (linePosEnd != -1) {
			const QString number = message.mid(
				linePos + 1,
				linePosEnd - linePos - 1
			).trimmed();
			bool ok = false;
			line = number.toInt(&ok) - 1;
			if (!ok) line = -1;
		}
	}
	return MessageInfo(
		fileName,
		line
	);
}
//END class Language

//BEGIN class ProcessOptionsSpecial
ProcessOptionsSpecial::ProcessOptionsSpecial() :
	b_addToProject(ProjectManager::self()->currentProject())
{
	switch (KTLConfig::hexFormat()) {
		case KTLConfig::EnumHexFormat::inhx8m:
			m_hexFormat = "inhx8m";
			break;

		case KTLConfig::EnumHexFormat::inhx8s:
			m_hexFormat = "inhx8s";
			break;

		case KTLConfig::EnumHexFormat::inhx16:
			m_hexFormat = "inhx16";
			break;

		case KTLConfig::EnumHexFormat::inhx32:
		default:
			m_hexFormat = "inhx32";
			break;
	}
}
//END class ProcessOptionsSpecial

//BEGIN class ProcessOptions
ProcessOptions::ProcessOptions(const OutputMethodInfo &info) :
	helper_(new ProcessOptionsHelper)
{
	setTargetFile(info.outputFile().path());

	Super::m_picID = info.picID();
	Super::b_addToProject = info.addToProject();

	switch (info.method()) {
		case OutputMethodInfo::Method::Direct:
			method_ = Method::LoadAsNew;
			break;

		default:
		case OutputMethodInfo::Method::SaveAndForget:
			method_ = Method::Forget;
			break;

		case OutputMethodInfo::Method::SaveAndLoad:
			method_ = Method::Load;
			break;
	}
}

void ProcessOptions::setTextOutputTarget(TextDocument *target, QObject *receiver, const char *slot) {
	textOutputTarget_ = target;
	QObject::connect(
		helper_,
		SIGNAL(textOutputtedTo(TextDocument *)),
		receiver,
		slot
	);
}

void ProcessOptions::setTextOutputtedTo(TextDocument *outputtedTo) {
	textOutputTarget_ = outputtedTo;
	emit helper_->textOutputtedTo(textOutputTarget_);
}

void ProcessOptions::setTargetFile(const QString &file) {
	if (targetFileSet_) {
		qWarning() << "Trying to reset target file!"<<endl;
		return;
	}
	targetFile_ = file;
	intermediaryFile_ = file;
	targetFileSet_ = true;
}

ProcessOptions::MediaType ProcessOptions::guessMediaType(const QString &url) {
	QString extension = url.right( url.length() - url.lastIndexOf('.') - 1 );
	extension = extension.toLower();

	// TODO : Replace with a hash-switch
	if (extension == "asm") {
		// We'll have to look at the file contents to determine its type...
		AsmParser p = AsmParser(url);
		p.parse();

		switch (p.type()) {
			case AsmParser::Type::Relocatable:
				return MediaType::AssemblyRelocatable;

			case AsmParser::Type::Absolute:
				return MediaType::AssemblyAbsolute;
		}
	}

	if (extension == "c")
		return MediaType::AssemblyRelocatable;

	if (extension == "flowcode")
		return MediaType::FlowCode;

	if (extension == "a" || extension == "lib")
		return MediaType::Library;

	if (extension == "microbe" || extension == "basic")
		return MediaType::Microbe;

	if (extension == "o")
		return MediaType::Object;

	if (extension == "hex")
		return MediaType::Program;

	return MediaType::Unknown;
}


ProcessOptions::Path ProcessOptions::path(MediaType from, MediaType to) {
	switch (from) {
		case MediaType::AssemblyAbsolute:
			switch (to) {
				case MediaType::AssemblyAbsolute:
					return Path::None;
				case MediaType::Pic:
					return Path::AssemblyAbsolute_PIC;
				case MediaType::Program:
					return Path::AssemblyAbsolute_Program;
				case MediaType::AssemblyRelocatable:
				case MediaType::C:
				case MediaType::Disassembly:
				case MediaType::FlowCode:
				case MediaType::Library:
				case MediaType::Microbe:
				case MediaType::Object:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::AssemblyRelocatable:
			switch (to) {
				case MediaType::Library:
					return Path::AssemblyRelocatable_Library;
				case MediaType::Object:
					return Path::AssemblyRelocatable_Object;
				case MediaType::Pic:
					return Path::AssemblyRelocatable_PIC;
				case MediaType::Program:
					return Path::AssemblyRelocatable_Program;
				case MediaType::AssemblyAbsolute:
				case MediaType::AssemblyRelocatable:
				case MediaType::C:
				case MediaType::Disassembly:
				case MediaType::FlowCode:
				case MediaType::Microbe:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::C:
			switch (to) {
				case MediaType::AssemblyRelocatable:
					return Path::C_AssemblyRelocatable;
				case MediaType::Library:
					return Path::C_Library;
				case MediaType::Object:
					return Path::C_Object;
				case MediaType::Pic:
					return Path::C_PIC;
				case MediaType::Program:
					return Path::C_Program;
				case MediaType::AssemblyAbsolute:
				case MediaType::C:
				case MediaType::Disassembly:
				case MediaType::FlowCode:
				case MediaType::Microbe:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::Disassembly:
			return Path::Invalid;

		case MediaType::FlowCode:
			switch (to) {
				case MediaType::AssemblyAbsolute:
					return Path::FlowCode_AssemblyAbsolute;
				case MediaType::Microbe:
					return Path::FlowCode_Microbe;
				case MediaType::Pic:
					return Path::FlowCode_PIC;
				case MediaType::Program:
					return Path::FlowCode_Program;
				case MediaType::AssemblyRelocatable:
				case MediaType::C:
				case MediaType::Disassembly:
				case MediaType::FlowCode:
				case MediaType::Library:
				case MediaType::Object:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::Library:
			return Path::Invalid;

		case MediaType::Microbe:
			switch (to) {
				case MediaType::AssemblyAbsolute:
					return Path::Microbe_AssemblyAbsolute;
				case MediaType::Pic:
					return Path::Microbe_PIC;
				case MediaType::Program:
					return Path::Microbe_Program;
				case MediaType::AssemblyRelocatable:
				case MediaType::C:
				case MediaType::Disassembly:
				case MediaType::FlowCode:
				case MediaType::Library:
				case MediaType::Microbe:
				case MediaType::Object:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::Object:
			switch (to) {
				case MediaType::Disassembly:
					return Path::Object_Disassembly;
				case MediaType::Library:
					return Path::Object_Library;
				case MediaType::Pic:
					return Path::Object_PIC;
				case MediaType::Program:
					return Path::Object_Program;
				case MediaType::AssemblyAbsolute:
				case MediaType::AssemblyRelocatable:
				case MediaType::C:
				case MediaType::FlowCode:
				case MediaType::Microbe:
				case MediaType::Object:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::Pic:
			return Path::Invalid;

		case MediaType::Program:
			switch (to) {
				case MediaType::Disassembly:
					return Path::Program_Disassembly;
				case MediaType::Pic:
					return Path::Program_PIC;
				case MediaType::AssemblyAbsolute:
				case MediaType::AssemblyRelocatable:
				case MediaType::C:
				case MediaType::FlowCode:
				case MediaType::Library:
				case MediaType::Microbe:
				case MediaType::Object:
				case MediaType::Program:
				case MediaType::Unknown:
					return Path::Invalid;
			}
			_fallthrough;

		case MediaType::Unknown:
			return Path::Invalid;
	}

	return Path::Invalid;
}


ProcessOptions::MediaType ProcessOptions::from(Path path) {
	switch (path) {
		case Path::AssemblyAbsolute_PIC:
		case Path::AssemblyAbsolute_Program:
			return MediaType::AssemblyAbsolute;

		case Path::AssemblyRelocatable_Library:
		case Path::AssemblyRelocatable_Object:
		case Path::AssemblyRelocatable_PIC:
		case Path::AssemblyRelocatable_Program:
		case Path::C_AssemblyRelocatable:
		case Path::C_Library:
			return MediaType::AssemblyRelocatable;

		case Path::FlowCode_AssemblyAbsolute:
		case Path::FlowCode_Microbe:
		case Path::FlowCode_PIC:
		case Path::FlowCode_Program:
			return MediaType::FlowCode;

		case Path::Microbe_AssemblyAbsolute:
		case Path::Microbe_PIC:
		case Path::Microbe_Program:
			return MediaType::Microbe;

		case Path::Object_Disassembly:
		case Path::Object_Library:
		case Path::Object_PIC:
		case Path::Object_Program:
		case Path::C_Object:
			return MediaType::Object;

		case Path::PIC_AssemblyAbsolute:
		case Path::C_PIC:
			return MediaType::Pic;

		case Path::Program_Disassembly:
		case Path::Program_PIC:
		case Path::C_Program:
			return MediaType::Program;

		case Path::Invalid:
		case Path::None:
			return MediaType::Unknown;
	}

	return MediaType::Unknown;
}


ProcessOptions::MediaType ProcessOptions::to(Path path) {
	switch (path) {
		case Path::FlowCode_AssemblyAbsolute:
		case Path::Microbe_AssemblyAbsolute:
		case Path::PIC_AssemblyAbsolute:
			return MediaType::AssemblyAbsolute;

		case Path::Object_Disassembly:
		case Path::Program_Disassembly:
			return MediaType::Disassembly;

		case Path::AssemblyRelocatable_Library:
		case Path::Object_Library:
		case Path::C_Library:
			return MediaType::Library;

		case Path::FlowCode_Microbe:
			return MediaType::Microbe;

		case Path::AssemblyRelocatable_Object:
		case Path::C_AssemblyRelocatable:
		case Path::C_Object:
			return MediaType::Object;

		case Path::AssemblyAbsolute_PIC:
		case Path::AssemblyRelocatable_PIC:
		case Path::FlowCode_PIC:
		case Path::Microbe_PIC:
		case Path::Object_PIC:
		case Path::Program_PIC:
		case Path::C_PIC:
			return MediaType::Pic;

		case Path::AssemblyAbsolute_Program:
		case Path::AssemblyRelocatable_Program:
		case Path::FlowCode_Program:
		case Path::Microbe_Program:
		case Path::Object_Program:
		case Path::C_Program:
			return MediaType::Program;

		case Path::Invalid:
		case Path::None:
			return MediaType::Unknown;
	}

	return MediaType::Unknown;
}
//END class ProcessOptions

#include "moc_language.cpp"
