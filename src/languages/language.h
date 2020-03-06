#pragma once

#include "pch.hpp"

#include <QObject>
#include <QStringList>

#include <atomic>

class FlowCodeDocument;
class KTechlab;
class LogView;
class MessageInfo;
class MicroSettings;
class OutputMethodInfo;
class ProcessChain;
class ProcessOptions;
class TextDocument;
class QProcess;

using ProcessOptionsList = QList<ProcessOptions>;

class ProcessOptionsSpecial {
	public:
		ProcessOptionsSpecial();

		QString m_picID;

		// Linking
		QString m_hexFormat;
		QString m_libraryDir;
		QString m_linkerScript;
		QStringList m_linkLibraries;
		QString m_linkOther;

		// Programming
		QString m_port;
		QString m_program;

		FlowCodeDocument *p_flowCodeDocument = nullptr;

		bool m_bOutputMapFile = false;
		bool b_addToProject = false;
		bool b_forceList = false;
};


class ProcessOptionsHelper final : public QObject {
	Q_OBJECT
	signals:
		void textOutputtedTo( TextDocument * outputtedTo );
};


class ProcessOptions final : public ProcessOptionsSpecial {
	using Super = ProcessOptionsSpecial;
	public:
		ProcessOptions() = default;
		ProcessOptions(const OutputMethodInfo &info);

		enum class MediaType {
			AssemblyAbsolute = 0,
			AssemblyRelocatable,
			C,
			Disassembly,
			FlowCode,
			Library,
			Microbe,
			Object,
			Pic,
			Program,

			Unknown // Used for guessing the media type
		};

		// From_To ::  processor that will be invoked first
		enum class Path {
			AssemblyAbsolute_PIC = 0,			// gpasm (indirect)
			AssemblyAbsolute_Program,		// gpasm (direct)

			AssemblyRelocatable_Library,	// gpasm (indirect)
			AssemblyRelocatable_Object,		// gpasm (direct)
			AssemblyRelocatable_PIC,		// gpasm (indirect)
			AssemblyRelocatable_Program,	// gpasm (indirect)

			C_AssemblyRelocatable,			// sdcc (direct)
			C_Library,						// sdcc (indirect)
			C_Object,						// sdcc (indirect)
			C_PIC,							// sdcc (indirect)
			C_Program,						// sdcc (indirect)

			FlowCode_AssemblyAbsolute,		// flowcode (indirect)
			FlowCode_Microbe,				// flowcode (direct)
			FlowCode_PIC,					// flowcode (indirect)
			FlowCode_Program,				// flowcode (indirect)

			Microbe_AssemblyAbsolute,		// microbe (direct)
			Microbe_PIC,					// microbe (indirect)
			Microbe_Program,				// microbe (indirect)

			Object_Disassembly,				// gpdasm (direct)
			Object_Library,					// gplib (direct)
			Object_PIC,						// gplink (indirect)
			Object_Program,					// gplink (direct)

			PIC_AssemblyAbsolute,			// download from pic (direct)

			Program_Disassembly,			// gpdasm (direct)
			Program_PIC,					// upload to pic (direct)

			Invalid, // From and to types are incompatible
			None // From and to types are the same
		};

		static Path path(MediaType from, MediaType to);
		static MediaType from(Path path);
		static MediaType to(Path path);

		enum class Method {
			Forget, // Don't do anything after processing successfully
			LoadAsNew, // Load the output as a new file
			Load // Load the output file
		};

		/**
		 * Tries to guess the media type from the url (and possible the contents
		 * of the file as well).
		 */
		static MediaType guessMediaType(const QString &url);
		/**
		 * The *final* target file (not any intermediatary ones)
		 */
		const QString & targetFile() const { return targetFile_; }
		/**
		 * This sets the final target file, as well as the initial intermediatary one
		 */
		void setTargetFile(const QString &file);

		void setIntermediaryOutput(const QString &file) { intermediaryFile_ = file; }
		const QString & intermediaryOutput() const { return intermediaryFile_; }

		void setInputFiles( const QStringList & files ) { inputFiles_ = files; }
		const QStringList & inputFiles() const { return inputFiles_; }

		void setMethod(Method method) { method_ = method; }
		Method method() const { return method_; }

		void setProcessPath(Path path) { processPath_ = path; }
		Path processPath() const { return processPath_; }

		/**
		 * If the output is text; If the user has selected (in config options)
		 * ReuseSameViewForOutput, then the given TextDocument will have its
		 * text set to the output if the TextDocument is not modified and has
		 * an empty url. Otherwise a new TextDocument will be created. Either
		 * way, once the processing has finished, a signal will be emitted
		 * to the given receiver passing a TextDocument * as an argument. This
		 * is not to be confused with setTextOutputtedTo, which is called once
		 * the processing has finished, and will call-back to the slot given.
		 */
		void setTextOutputTarget(TextDocument *target, QObject *receiver, const char *slot);
		/**
		 * @see setTextOutputTarget
		 */
		TextDocument * textOutputTarget() const { return textOutputTarget_; }
		/**
		 * @see setTextOuputTarget
		 */
		void setTextOutputtedTo(TextDocument *outputtedTo);

	protected:
		QStringList inputFiles_;
		QString targetFile_;
		QString intermediaryFile_;
		TextDocument *textOutputTarget_ = nullptr;
		// TODO : Do we need to delete this?
		ProcessOptionsHelper *helper_ = nullptr;
		Method method_ = Method::Forget;
		Path processPath_ = Path::None;
		bool targetFileSet_ = false;
};


/**
@author Daniel Clarke
@author David Saxton
*/
class Language : public QObject {
	Q_OBJECT
	protected:
		Language(
			ProcessChain *processChain,
			const QString &name,
			const QString &successMessage = QString{},
			const QString &failureMessage = QString{}
		);
	public:
		~Language() override = default;

		// Compile / assemble / dissassembly / whatever the given input.
		virtual void processInput(const ProcessOptions &options) = 0;

		// Return the ProcessOptions object current state
		const ProcessOptions & processOptions() const { return processOptions_; }

		/**
		 * Return the output path from the given input path. Will return None
		 * if we've done processing.
		 */
		virtual ProcessOptions::Path outputPath(ProcessOptions::Path inputPath) const = 0;

	signals:
		/**
		 * Emitted when the processing was successful.
		 * @param language Pointer to this class
		 */
		void processSucceeded(Language *language);
		/**
		 * Emitted when the processing failed.
		 * @param language Pointer to this class
		 */
		void processFailed(Language *language);

	protected:
		/**
		 * Examines the string for the line number if applicable, and creates a new
		 * MessageInfo for it.
		 */
		virtual MessageInfo extractMessageInfo(const QString &text);

		// Reset the error count
		void reset();
		void outputMessage(const QString &message);
		void outputWarning(const QString &message);
		void outputError(const QString &error);
		void finish(Result result);

		ProcessOptions processOptions_;
		// A message appropriate to the language's success after compilation or similar.
		const QString successMessage_;
		// A message appropriate to the language's failure after compilation or similar.
		const QString failureMessage_;
		ProcessChain * const processChain_ = nullptr;

		std::atomic<int> errorCount_ = { 0 };
};
